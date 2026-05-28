"""OCR service — extracts expiry date from food package images."""

import re
from datetime import date, timedelta
from typing import Optional

import pytesseract
from PIL import Image

from app.config import settings

pytesseract.pytesseract.tesseract_cmd = settings.tesseract_cmd

_DATE_PATTERNS = [
    r"(20\d{2})[.\-/](\d{1,2})[.\-/](\d{1,2})",
    r"(\d{2})[.\-/](\d{1,2})[.\-/](\d{1,2})",
    r"(20\d{2})(\d{2})(\d{2})",
    r"(20\d{2})\s*년\s*(\d{1,2})\s*월\s*(\d{1,2})\s*일",
]

_EXPIRY_KEYWORDS = ["유통기한", "소비기한", "까지", "best before", "exp", "use by"]


def _parse_date(year: str, month: str, day: str) -> Optional[date]:
    try:
        y = int(year)
        if y < 100:
            y += 2000
        return date(y, int(month), int(day))
    except ValueError:
        return None


def _find_best_date(text: str) -> Optional[date]:
    """Find the most likely expiry date in OCR text."""
    today = date.today()
    two_years_later = today + timedelta(days=730)

    candidates: list[date] = []
    for pattern in _DATE_PATTERNS:
        for match in re.finditer(pattern, text, re.IGNORECASE):
            g = match.groups()
            parsed = _parse_date(g[0], g[1], g[2])
            if parsed and today < parsed <= two_years_later:
                candidates.append(parsed)

    if not candidates:
        return None

    # prefer the date closest to an expiry keyword
    lower = text.lower()
    for keyword in _EXPIRY_KEYWORDS:
        idx = lower.find(keyword)
        if idx == -1:
            continue
        # find the nearest candidate date by character position
        for pattern in _DATE_PATTERNS:
            for match in re.finditer(pattern, text, re.IGNORECASE):
                g = match.groups()
                parsed = _parse_date(g[0], g[1], g[2])
                if parsed in candidates:
                    return parsed

    # fallback: return the soonest expiry date
    return min(candidates)


def extract_expiry_date(image_bytes: bytes) -> dict:
    """
    Returns:
        {"success": True, "expiry_date": date, "raw_text": str}
        {"success": False, "error": str, "raw_text": str}
    """
    try:
        import io
        img = Image.open(io.BytesIO(image_bytes))

        # Try Korean + English first, fall back to English only
        raw = pytesseract.image_to_string(img, lang="kor+eng")
        if not raw.strip():
            raw = pytesseract.image_to_string(img, lang="eng")

        expiry = _find_best_date(raw)
        if expiry:
            return {"success": True, "expiry_date": expiry, "raw_text": raw}
        return {"success": False, "error": "날짜를 인식하지 못했습니다", "raw_text": raw}

    except Exception as e:
        return {"success": False, "error": str(e), "raw_text": ""}
