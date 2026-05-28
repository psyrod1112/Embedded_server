"""Stock-in / stock-out endpoints (RPi-facing + app confirmation)."""

import json
import uuid
from datetime import date, datetime
from typing import List

from fastapi import APIRouter, Depends, File, HTTPException, UploadFile
from sqlalchemy import func
from sqlalchemy.orm import Session

from app.auth import get_current_user
from app.database import get_db
from app.models import FoodItem, FoodType, PendingStockOut, User, WeightLog
from app.schemas import (
    ScanResult,
    StockInRequest,
    StockInResponse,
    StockOutCandidate,
    StockOutConfirm,
    StockOutRequest,
    StockOutResponse,
    FoodItemOut,
)
from app.services import fcm as fcm_service
from app.services.ocr import extract_expiry_date

router = APIRouter(prefix="/stock", tags=["stock"])

_WEIGHT_THRESHOLD = 10.0   # grams — ignore smaller deltas


def _get_fcm_token(db: Session) -> str | None:
    user = db.query(User).filter(User.fcm_token.isnot(None)).first()
    return user.fcm_token if user else None


def _fifo_position(db: Session, type_id: uuid.UUID, expiry_date: date) -> int:
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


def _count_types(db: Session) -> int:
    return db.query(FoodType).filter(FoodType.deleted_at.is_(None)).count()


def _find_matching_types(db: Session, delta_gram: float) -> List[FoodType]:
    """Return food types whose unit_weight_gram is within ±tolerance of delta_gram."""
    types = db.query(FoodType).filter(FoodType.deleted_at.is_(None)).all()
    matches = []
    for ft in types:
        low = ft.unit_weight_gram * (1 - ft.weight_tolerance_pct / 100)
        high = ft.unit_weight_gram * (1 + ft.weight_tolerance_pct / 100)
        if low <= delta_gram <= high:
            matches.append(ft)
    return sorted(matches, key=lambda t: abs(t.unit_weight_gram - delta_gram))


# ── OCR Scan ──────────────────────────────────────────────────────────────────

@router.post("/scan", response_model=ScanResult)
async def scan_image(file: UploadFile = File(...), _: User = Depends(get_current_user)):
    image_bytes = await file.read()
    result = extract_expiry_date(image_bytes)
    return ScanResult(
        success=result["success"],
        expiry_date=result.get("expiry_date"),
        raw_text=result.get("raw_text"),
        error=result.get("error"),
    )


# ── Stock-In ──────────────────────────────────────────────────────────────────

@router.post("/in", response_model=StockInResponse)
def stock_in(
    body: StockInRequest,
    db: Session = Depends(get_db),
    _: User = Depends(get_current_user),
):
    if body.delta_gram < _WEIGHT_THRESHOLD:
        raise HTTPException(status_code=400, detail="무게 변화가 임계값 이하입니다")

    matches = _find_matching_types(db, body.delta_gram)
    is_new_type = len(matches) == 0

    if is_new_type:
        count = _count_types(db)
        ft = FoodType(
            type_name=f"Food_{count + 1}",
            unit_weight_gram=body.delta_gram,
        )
        db.add(ft)
        db.flush()
        quantity = 1
    else:
        ft = matches[0]
        quantity = max(1, round(body.delta_gram / ft.unit_weight_gram))

    pos_start = _fifo_position(db, ft.type_id, body.expiry_date)
    pos_end = pos_start + quantity - 1

    item = FoodItem(
        type_id=ft.type_id,
        expiry_date=body.expiry_date,
        quantity=quantity,
        weight_gram=ft.unit_weight_gram,
    )
    db.add(item)
    db.add(WeightLog(event_type="IN", delta_gram=body.delta_gram, type_id=ft.type_id, item_id=item.item_id))
    db.commit()
    db.refresh(ft)

    fcm_token = _get_fcm_token(db)
    if fcm_token:
        fcm_service.send_stock_in_notification(
            fcm_token, ft.type_name, quantity, str(body.expiry_date)
        )

    return StockInResponse(
        type_id=ft.type_id,
        type_name=ft.type_name,
        quantity=quantity,
        fifo_position=pos_start,
        fifo_position_end=pos_end,
        expiry_date=body.expiry_date,
        is_new_type=is_new_type,
    )


# ── Stock-Out ─────────────────────────────────────────────────────────────────

@router.post("/out", response_model=StockOutResponse)
def stock_out(
    body: StockOutRequest,
    db: Session = Depends(get_db),
    _: User = Depends(get_current_user),
):
    if body.delta_gram < _WEIGHT_THRESHOLD:
        raise HTTPException(status_code=400, detail="무게 변화가 임계값 이하입니다")

    matches = _find_matching_types(db, body.delta_gram)
    fcm_token = _get_fcm_token(db)

    if not matches:
        if fcm_token:
            fcm_service.send_stock_out_no_match(fcm_token, body.delta_gram)
        db.add(WeightLog(event_type="OUT", delta_gram=body.delta_gram))
        db.commit()
        return StockOutResponse(auto_resolved=False, error="매칭 식품 없음. 수동 처리 필요.")

    if len(matches) == 1:
        return _do_auto_stock_out(db, matches[0], body.delta_gram, fcm_token)

    # Multiple candidates — send to app
    candidates = []
    for ft in matches[:5]:
        oldest = (
            db.query(FoodItem)
            .filter(FoodItem.type_id == ft.type_id, FoodItem.deleted_at.is_(None))
            .order_by(FoodItem.expiry_date.asc())
            .first()
        )
        if oldest:
            diff_pct = abs(ft.unit_weight_gram - body.delta_gram) / ft.unit_weight_gram * 100
            candidates.append(
                StockOutCandidate(
                    type_id=ft.type_id,
                    type_name=ft.type_name,
                    item_id=oldest.item_id,
                    expiry_date=oldest.expiry_date,
                    unit_weight_gram=ft.unit_weight_gram,
                    weight_diff_pct=round(diff_pct, 1),
                )
            )

    pending = PendingStockOut(
        delta_gram=body.delta_gram,
        candidates=json.dumps([c.model_dump(mode="json") for c in candidates]),
    )
    db.add(pending)
    db.commit()
    db.refresh(pending)

    if fcm_token:
        fcm_service.send_stock_out_candidates(fcm_token, body.delta_gram, str(pending.event_id))

    return StockOutResponse(
        auto_resolved=False,
        candidates=candidates,
        pending_event_id=pending.event_id,
    )


def _do_auto_stock_out(db: Session, ft: FoodType, delta_gram: float, fcm_token) -> StockOutResponse:
    oldest = (
        db.query(FoodItem)
        .filter(FoodItem.type_id == ft.type_id, FoodItem.deleted_at.is_(None))
        .order_by(FoodItem.expiry_date.asc())
        .first()
    )
    if not oldest:
        return StockOutResponse(auto_resolved=False, error="재고 없음")

    if oldest.quantity > 1:
        oldest.quantity -= 1
    else:
        oldest.deleted_at = datetime.utcnow()

    db.add(WeightLog(event_type="OUT", delta_gram=delta_gram, type_id=ft.type_id, item_id=oldest.item_id))
    db.commit()
    db.refresh(oldest)

    removed = FoodItemOut(
        item_id=oldest.item_id,
        type_id=oldest.type_id,
        type_name=ft.type_name,
        expiry_date=oldest.expiry_date,
        quantity=oldest.quantity,
        weight_gram=oldest.weight_gram,
        registered_at=oldest.registered_at,
        fifo_position=1,
    )
    return StockOutResponse(auto_resolved=True, removed_item=removed)


# ── App Confirms Stock-Out Candidate ──────────────────────────────────────────

@router.post("/out/confirm")
def confirm_stock_out(
    body: StockOutConfirm,
    db: Session = Depends(get_db),
    _: User = Depends(get_current_user),
):
    pending = db.query(PendingStockOut).filter(
        PendingStockOut.event_id == body.pending_event_id,
        PendingStockOut.status == "PENDING",
    ).first()
    if not pending:
        raise HTTPException(status_code=404, detail="대기 이벤트를 찾을 수 없습니다")

    ft = db.query(FoodType).filter(FoodType.type_id == body.type_id).first()
    if not ft:
        raise HTTPException(status_code=404, detail="타입을 찾을 수 없습니다")

    result = _do_auto_stock_out(db, ft, pending.delta_gram, None)
    pending.status = "CONFIRMED"
    pending.resolved_at = datetime.utcnow()
    db.commit()
    return {"message": "출고 확정 완료", "removed": result.removed_item}


# ── Pending Events List ───────────────────────────────────────────────────────

@router.get("/pending")
def list_pending(
    db: Session = Depends(get_db),
    _: User = Depends(get_current_user),
):
    events = (
        db.query(PendingStockOut)
        .filter(PendingStockOut.status == "PENDING")
        .order_by(PendingStockOut.created_at.desc())
        .all()
    )
    return [
        {
            "event_id": e.event_id,
            "delta_gram": e.delta_gram,
            "candidates": json.loads(e.candidates),
            "created_at": e.created_at,
        }
        for e in events
    ]
