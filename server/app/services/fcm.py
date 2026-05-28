"""Firebase Cloud Messaging service."""

import json
import logging
from typing import Optional

import firebase_admin
from firebase_admin import credentials, messaging

from app.config import settings

logger = logging.getLogger(__name__)

_initialized = False


def _init():
    global _initialized
    if not _initialized:
        cred = credentials.Certificate(settings.firebase_credentials_path)
        firebase_admin.initialize_app(cred)
        _initialized = True


def send_notification(fcm_token: str, title: str, body: str, data: Optional[dict] = None) -> bool:
    """Send FCM push notification. Returns True on success."""
    try:
        _init()
        message = messaging.Message(
            notification=messaging.Notification(title=title, body=body),
            data={k: str(v) for k, v in (data or {}).items()},
            token=fcm_token,
        )
        messaging.send(message)
        return True
    except Exception as e:
        logger.error("FCM send failed: %s", e)
        return False


def send_stock_in_notification(fcm_token: str, type_name: str, quantity: int, expiry_date: str):
    send_notification(
        fcm_token,
        title="입고 완료",
        body=f"{type_name} {quantity}개 등록됨 (유통기한: {expiry_date}). 수량/타입 확인하세요.",
        data={"type": "STOCK_IN"},
    )


def send_stock_out_candidates(fcm_token: str, delta_gram: float, pending_event_id: str):
    send_notification(
        fcm_token,
        title="출고 확인 필요",
        body=f"{delta_gram:.0f}g 감소 감지. 출고 식품을 확인해주세요.",
        data={"type": "STOCK_OUT_CANDIDATES", "pending_event_id": pending_event_id},
    )


def send_stock_out_no_match(fcm_token: str, delta_gram: float):
    send_notification(
        fcm_token,
        title="출고 확인 불가",
        body=f"{delta_gram:.0f}g 감소했으나 매칭 식품 없음. 수동 처리 필요.",
        data={"type": "STOCK_OUT_NO_MATCH"},
    )


def send_expiry_alert(fcm_token: str, count: int):
    send_notification(
        fcm_token,
        title="유통기한 임박 알림",
        body=f"유통기한 3일 이내 식품 {count}개 있습니다.",
        data={"type": "EXPIRY_ALERT"},
    )
