"""Food type & item management endpoints (app-facing)."""

import uuid
from datetime import datetime
from typing import List

from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy import func
from sqlalchemy.orm import Session

from app.auth import get_current_user
from app.database import get_db
from app.models import FoodItem, FoodType, User, WeightLog
from app.schemas import (
    FoodItemOut,
    FoodTypeCreate,
    FoodTypeOut,
    FoodTypeRename,
    ManualStockIn,
    ManualStockOut,
    RestoreRequest,
)

router = APIRouter(prefix="/foods", tags=["foods"])


def _fifo_position(db: Session, type_id: uuid.UUID, expiry_date) -> int:
    """1-based rank of expiry_date among active items of same type (ascending)."""
    earlier = (
        db.query(func.sum(FoodItem.quantity))
        .filter(
            FoodItem.type_id == type_id,
            FoodItem.expiry_date < expiry_date,
            FoodItem.deleted_at.is_(None),
        )
        .scalar()
        or 0
    )
    return int(earlier) + 1


def _type_total(db: Session, type_id: uuid.UUID) -> int:
    return int(
        db.query(func.sum(FoodItem.quantity))
        .filter(FoodItem.type_id == type_id, FoodItem.deleted_at.is_(None))
        .scalar()
        or 0
    )


# ── Food Types ─────────────────────────────────────────────────────────────────

@router.get("/types", response_model=List[FoodTypeOut])
def list_types(
    db: Session = Depends(get_db),
    _: User = Depends(get_current_user),
):
    types = db.query(FoodType).filter(FoodType.deleted_at.is_(None)).all()
    result = []
    for ft in types:
        total = _type_total(db, ft.type_id)
        result.append(
            FoodTypeOut(
                type_id=ft.type_id,
                type_name=ft.type_name,
                unit_weight_gram=ft.unit_weight_gram,
                weight_tolerance_pct=ft.weight_tolerance_pct,
                total_quantity=total,
                created_at=ft.created_at,
            )
        )
    return result


@router.post("/types", response_model=FoodTypeOut, status_code=201)
def create_type(
    body: FoodTypeCreate,
    db: Session = Depends(get_db),
    _: User = Depends(get_current_user),
):
    ft = FoodType(
        type_name=body.type_name,
        unit_weight_gram=body.unit_weight_gram,
        weight_tolerance_pct=body.weight_tolerance_pct,
    )
    db.add(ft)
    db.commit()
    db.refresh(ft)
    return FoodTypeOut(
        type_id=ft.type_id,
        type_name=ft.type_name,
        unit_weight_gram=ft.unit_weight_gram,
        weight_tolerance_pct=ft.weight_tolerance_pct,
        total_quantity=0,
        created_at=ft.created_at,
    )


@router.patch("/types/{type_id}", response_model=FoodTypeOut)
def rename_type(
    type_id: uuid.UUID,
    body: FoodTypeRename,
    db: Session = Depends(get_db),
    _: User = Depends(get_current_user),
):
    ft = db.query(FoodType).filter(FoodType.type_id == type_id, FoodType.deleted_at.is_(None)).first()
    if not ft:
        raise HTTPException(status_code=404, detail="타입을 찾을 수 없습니다")
    ft.type_name = body.type_name
    db.commit()
    db.refresh(ft)
    return FoodTypeOut(
        type_id=ft.type_id,
        type_name=ft.type_name,
        unit_weight_gram=ft.unit_weight_gram,
        weight_tolerance_pct=ft.weight_tolerance_pct,
        total_quantity=_type_total(db, ft.type_id),
        created_at=ft.created_at,
    )


# ── Food Items ─────────────────────────────────────────────────────────────────

@router.get("/items", response_model=List[FoodItemOut])
def list_items(
    type_id: uuid.UUID,
    db: Session = Depends(get_db),
    _: User = Depends(get_current_user),
):
    items = (
        db.query(FoodItem)
        .filter(FoodItem.type_id == type_id, FoodItem.deleted_at.is_(None))
        .order_by(FoodItem.expiry_date.asc())
        .all()
    )
    result = []
    pos = 1
    for item in items:
        ft = db.query(FoodType).filter(FoodType.type_id == item.type_id).first()
        result.append(
            FoodItemOut(
                item_id=item.item_id,
                type_id=item.type_id,
                type_name=ft.type_name if ft else "",
                expiry_date=item.expiry_date,
                quantity=item.quantity,
                weight_gram=item.weight_gram,
                registered_at=item.registered_at,
                fifo_position=pos,
            )
        )
        pos += item.quantity
    return result


# ── Manual Stock-In ────────────────────────────────────────────────────────────

@router.post("/items/manual-in", response_model=FoodItemOut)
def manual_stock_in(
    body: ManualStockIn,
    db: Session = Depends(get_db),
    _: User = Depends(get_current_user),
):
    ft = db.query(FoodType).filter(FoodType.type_id == body.type_id, FoodType.deleted_at.is_(None)).first()
    if not ft:
        raise HTTPException(status_code=404, detail="타입을 찾을 수 없습니다")

    pos = _fifo_position(db, body.type_id, body.expiry_date)
    item = FoodItem(
        type_id=body.type_id,
        expiry_date=body.expiry_date,
        quantity=body.quantity,
        weight_gram=body.weight_gram,
    )
    db.add(item)
    db.add(WeightLog(event_type="IN", delta_gram=body.weight_gram * body.quantity, type_id=body.type_id, item_id=item.item_id))
    db.commit()
    db.refresh(item)
    return FoodItemOut(
        item_id=item.item_id,
        type_id=item.type_id,
        type_name=ft.type_name,
        expiry_date=item.expiry_date,
        quantity=item.quantity,
        weight_gram=item.weight_gram,
        registered_at=item.registered_at,
        fifo_position=pos,
    )


# ── Manual Stock-Out ───────────────────────────────────────────────────────────

@router.post("/items/manual-out")
def manual_stock_out(
    body: ManualStockOut,
    db: Session = Depends(get_db),
    _: User = Depends(get_current_user),
):
    item = db.query(FoodItem).filter(FoodItem.item_id == body.item_id, FoodItem.deleted_at.is_(None)).first()
    if not item:
        raise HTTPException(status_code=404, detail="식품을 찾을 수 없습니다")

    if item.quantity > 1:
        item.quantity -= 1
    else:
        item.deleted_at = datetime.utcnow()

    db.add(WeightLog(event_type="OUT", delta_gram=item.weight_gram, type_id=item.type_id, item_id=item.item_id))
    db.commit()
    return {"message": "출고 완료"}


# ── Restore (Undo) ─────────────────────────────────────────────────────────────

@router.post("/items/restore")
def restore_item(
    body: RestoreRequest,
    db: Session = Depends(get_db),
    _: User = Depends(get_current_user),
):
    item = db.query(FoodItem).filter(FoodItem.item_id == body.item_id).first()
    if not item:
        raise HTTPException(status_code=404, detail="식품을 찾을 수 없습니다")

    if item.deleted_at is None:
        item.quantity += 1
    else:
        item.deleted_at = None

    db.commit()
    return {"message": "복원 완료"}
