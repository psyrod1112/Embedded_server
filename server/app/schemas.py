from __future__ import annotations

import uuid
from datetime import date, datetime
from typing import List, Optional

from pydantic import BaseModel


# ── Auth ──────────────────────────────────────────────────────────────────────

class LoginRequest(BaseModel):
    username: str
    password: str


class TokenResponse(BaseModel):
    access_token: str
    token_type: str = "bearer"


class FcmTokenUpdate(BaseModel):
    fcm_token: str


# ── Food Types ────────────────────────────────────────────────────────────────

class FoodTypeOut(BaseModel):
    type_id: uuid.UUID
    type_name: str
    unit_weight_gram: float
    weight_tolerance_pct: float
    total_quantity: int
    created_at: datetime

    class Config:
        from_attributes = True


class FoodTypeCreate(BaseModel):
    type_name: str
    unit_weight_gram: float
    weight_tolerance_pct: float = 15.0


class FoodTypeRename(BaseModel):
    type_name: str


# ── Food Items ────────────────────────────────────────────────────────────────

class FoodItemOut(BaseModel):
    item_id: uuid.UUID
    type_id: uuid.UUID
    type_name: str
    expiry_date: date
    quantity: int
    weight_gram: float
    registered_at: datetime
    fifo_position: int           # rank among items of same type by expiry_date

    class Config:
        from_attributes = True


class ManualStockIn(BaseModel):
    type_id: uuid.UUID
    expiry_date: date
    quantity: int = 1
    weight_gram: float


class ManualStockOut(BaseModel):
    item_id: uuid.UUID


# ── Stock Events (RPi → Server) ───────────────────────────────────────────────

class ScanResult(BaseModel):
    success: bool
    expiry_date: Optional[date] = None
    raw_text: Optional[str] = None
    error: Optional[str] = None


class StockInRequest(BaseModel):
    expiry_date: date
    delta_gram: float            # total weight increase


class StockInResponse(BaseModel):
    type_id: uuid.UUID
    type_name: str
    quantity: int
    fifo_position: int           # position of first new item
    fifo_position_end: int       # position of last new item (same as start if qty=1)
    expiry_date: date
    is_new_type: bool


class StockOutRequest(BaseModel):
    delta_gram: float            # weight decrease (positive value)


class StockOutCandidate(BaseModel):
    type_id: uuid.UUID
    type_name: str
    item_id: uuid.UUID
    expiry_date: date
    unit_weight_gram: float
    weight_diff_pct: float       # how close to delta


class StockOutResponse(BaseModel):
    auto_resolved: bool
    removed_item: Optional[FoodItemOut] = None
    candidates: Optional[List[StockOutCandidate]] = None
    pending_event_id: Optional[uuid.UUID] = None
    error: Optional[str] = None


class StockOutConfirm(BaseModel):
    pending_event_id: uuid.UUID
    type_id: uuid.UUID


# ── Restore ───────────────────────────────────────────────────────────────────

class RestoreRequest(BaseModel):
    item_id: uuid.UUID
