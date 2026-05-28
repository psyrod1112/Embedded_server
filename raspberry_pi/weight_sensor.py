"""HX711 weight sensor interface."""

import logging
import time

from config import HX711_DOUT, HX711_SCK

logger = logging.getLogger(__name__)

try:
    from hx711 import HX711 as _HX711
    _HW_AVAILABLE = True
except ImportError:
    _HW_AVAILABLE = False
    logger.warning("hx711 library not found — using simulated weight sensor")


class WeightSensor:
    # Calibration factor — adjust after calibration with known weights
    CALIBRATION_FACTOR = -7050

    def __init__(self):
        self._hx = None
        if _HW_AVAILABLE:
            self._hx = _HX711(HX711_DOUT, HX711_SCK)
            self._hx.set_reading_format("MSB", "MSB")
            self._hx.set_reference_unit(self.CALIBRATION_FACTOR)
            self._hx.reset()
            self._hx.tare()
            logger.info("HX711 initialized and tared")

    def read_grams(self, samples: int = 5) -> float:
        """Return current weight in grams (averaged over N samples)."""
        if not self._hx:
            return 0.0
        readings = []
        for _ in range(samples):
            val = self._hx.get_weight(1)
            readings.append(val)
            time.sleep(0.05)
        self._hx.power_down()
        self._hx.power_up()
        return sum(readings) / len(readings)

    def tare(self):
        """Reset zero point."""
        if self._hx:
            self._hx.tare()
            logger.info("Weight sensor tared")
