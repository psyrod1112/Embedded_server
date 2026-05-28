"""HC-SR04 ultrasonic sensor — door open detection."""

import logging
import time
import threading
from typing import Callable, Optional

from config import (
    DOOR_OPEN_ALERT_SEC,
    DOOR_OPEN_DISTANCE_CM,
    ULTRASONIC_ECHO,
    ULTRASONIC_TRIG,
)

logger = logging.getLogger(__name__)

try:
    import RPi.GPIO as GPIO
    _HW_AVAILABLE = True
except ImportError:
    _HW_AVAILABLE = False
    logger.warning("RPi.GPIO not found — ultrasonic sensor simulated")


class UltrasonicSensor:
    POLL_INTERVAL = 0.5   # seconds between measurements

    def __init__(self):
        self._running = False
        self._thread: Optional[threading.Thread] = None
        self._on_door_open_long: Optional[Callable] = None
        self._door_open_since: Optional[float] = None
        self._alert_sent = False

        if _HW_AVAILABLE:
            GPIO.setmode(GPIO.BCM)
            GPIO.setup(ULTRASONIC_TRIG, GPIO.OUT)
            GPIO.setup(ULTRASONIC_ECHO, GPIO.IN)
            GPIO.output(ULTRASONIC_TRIG, False)
            time.sleep(0.1)

    def start(self, on_door_open_long: Callable):
        """on_door_open_long fires when door stays open > DOOR_OPEN_ALERT_SEC."""
        self._on_door_open_long = on_door_open_long
        self._running = True
        self._thread = threading.Thread(target=self._poll_loop, daemon=True)
        self._thread.start()

    def stop(self):
        self._running = False
        if _HW_AVAILABLE:
            GPIO.cleanup()

    def _measure_cm(self) -> float:
        if not _HW_AVAILABLE:
            return 30.0   # simulate door closed
        GPIO.output(ULTRASONIC_TRIG, True)
        time.sleep(0.00001)
        GPIO.output(ULTRASONIC_TRIG, False)

        pulse_start = time.time()
        while GPIO.input(ULTRASONIC_ECHO) == 0:
            pulse_start = time.time()
        pulse_end = time.time()
        while GPIO.input(ULTRASONIC_ECHO) == 1:
            pulse_end = time.time()

        return (pulse_end - pulse_start) * 17150  # cm

    def _poll_loop(self):
        while self._running:
            try:
                dist = self._measure_cm()
                is_open = dist < DOOR_OPEN_DISTANCE_CM

                if is_open:
                    if self._door_open_since is None:
                        self._door_open_since = time.time()
                        self._alert_sent = False
                    elif not self._alert_sent:
                        elapsed = time.time() - self._door_open_since
                        if elapsed >= DOOR_OPEN_ALERT_SEC:
                            self._alert_sent = True
                            if self._on_door_open_long:
                                self._on_door_open_long()
                else:
                    self._door_open_since = None
                    self._alert_sent = False

            except Exception as e:
                logger.error("Ultrasonic read error: %s", e)

            time.sleep(self.POLL_INTERVAL)
