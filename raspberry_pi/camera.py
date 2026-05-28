"""Camera capture — grabs a JPEG frame and returns raw bytes."""

import io
import logging
import time

logger = logging.getLogger(__name__)

try:
    from picamera2 import Picamera2
    _HW_AVAILABLE = True
except ImportError:
    _HW_AVAILABLE = False
    logger.warning("picamera2 not found — camera simulated")


class Camera:
    def __init__(self):
        self._cam = None
        if _HW_AVAILABLE:
            self._cam = Picamera2()
            config = self._cam.create_still_configuration(main={"size": (1920, 1080)})
            self._cam.configure(config)
            self._cam.start()
            time.sleep(2)   # warm-up

    def capture_jpeg(self) -> bytes:
        """Return JPEG bytes of the current frame."""
        if not self._cam:
            return b""
        buf = io.BytesIO()
        self._cam.capture_file(buf, format="jpeg")
        return buf.getvalue()

    def close(self):
        if self._cam:
            self._cam.stop()
