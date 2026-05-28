"""HTTP client for the AWS FastAPI server."""

import logging
from datetime import date
from typing import Optional

import httpx

from config import API_TOKEN, SERVER_URL

logger = logging.getLogger(__name__)

_HEADERS = {"Authorization": f"Bearer {API_TOKEN}"}
_TIMEOUT = 15.0


def scan_image(jpeg_bytes: bytes) -> dict:
    """POST /stock/scan — returns {success, expiry_date, error}."""
    try:
        resp = httpx.post(
            f"{SERVER_URL}/stock/scan",
            files={"file": ("frame.jpg", jpeg_bytes, "image/jpeg")},
            headers=_HEADERS,
            timeout=_TIMEOUT,
        )
        resp.raise_for_status()
        return resp.json()
    except Exception as e:
        logger.error("scan_image failed: %s", e)
        return {"success": False, "error": str(e)}


def stock_in(expiry_date: date, delta_gram: float) -> Optional[dict]:
    """POST /stock/in — returns StockInResponse dict."""
    try:
        resp = httpx.post(
            f"{SERVER_URL}/stock/in",
            json={"expiry_date": str(expiry_date), "delta_gram": delta_gram},
            headers=_HEADERS,
            timeout=_TIMEOUT,
        )
        resp.raise_for_status()
        return resp.json()
    except Exception as e:
        logger.error("stock_in failed: %s", e)
        return None


def stock_out(delta_gram: float) -> Optional[dict]:
    """POST /stock/out — returns StockOutResponse dict."""
    try:
        resp = httpx.post(
            f"{SERVER_URL}/stock/out",
            json={"delta_gram": delta_gram},
            headers=_HEADERS,
            timeout=_TIMEOUT,
        )
        resp.raise_for_status()
        return resp.json()
    except Exception as e:
        logger.error("stock_out failed: %s", e)
        return None
