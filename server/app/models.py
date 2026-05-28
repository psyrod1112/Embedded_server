import uuid
from datetime import datetime

from sqlalchemy import Column, String, Float, Integer, DateTime, Date, ForeignKey, Enum, Text
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.orm import relationship

from app.database import Base


class User(Base):
    __tablename__ = "users"

    user_id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    username = Column(String(50), unique=True, nullable=False)
    password_hash = Column(String(255), nullable=False)
    fcm_token = Column(String(512), nullable=True)
    created_at = Column(DateTime, default=datetime.utcnow)


class FoodType(Base):
    __tablename__ = "food_types"

    type_id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    type_name = Column(String(100), nullable=False)       # "Food_1" or user-renamed
    unit_weight_gram = Column(Float, nullable=False)      # representative unit weight
    weight_tolerance_pct = Column(Float, default=15.0)   # ±% for matching
    created_at = Column(DateTime, default=datetime.utcnow)
    deleted_at = Column(DateTime, nullable=True)

    items = relationship("FoodItem", back_populates="food_type")


class FoodItem(Base):
    """One row per stock-in batch. quantity tracks count, expiry_date drives FIFO."""
    __tablename__ = "food_items"

    item_id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    type_id = Column(UUID(as_uuid=True), ForeignKey("food_types.type_id"), nullable=False)
    expiry_date = Column(Date, nullable=False)
    quantity = Column(Integer, default=1, nullable=False)
    weight_gram = Column(Float, nullable=False)           # measured delta / quantity
    registered_at = Column(DateTime, default=datetime.utcnow)
    deleted_at = Column(DateTime, nullable=True)

    food_type = relationship("FoodType", back_populates="items")
    weight_logs = relationship("WeightLog", back_populates="food_item")


class WeightLog(Base):
    __tablename__ = "weight_logs"

    log_id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    event_type = Column(Enum("IN", "OUT", "ERROR", name="event_type_enum"), nullable=False)
    delta_gram = Column(Float, nullable=False)
    type_id = Column(UUID(as_uuid=True), ForeignKey("food_types.type_id"), nullable=True)
    item_id = Column(UUID(as_uuid=True), ForeignKey("food_items.item_id"), nullable=True)
    created_at = Column(DateTime, default=datetime.utcnow)

    food_item = relationship("FoodItem", back_populates="weight_logs")


class PendingStockOut(Base):
    """Created when stock-out has multiple matching candidates. App confirms."""
    __tablename__ = "pending_stock_outs"

    event_id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    delta_gram = Column(Float, nullable=False)
    candidates = Column(Text, nullable=False)             # JSON string
    status = Column(
        Enum("PENDING", "CONFIRMED", "CANCELLED", name="pending_status_enum"),
        default="PENDING",
    )
    created_at = Column(DateTime, default=datetime.utcnow)
    resolved_at = Column(DateTime, nullable=True)
