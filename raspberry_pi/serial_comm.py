"""Serial communication with Arduino — JSON Lines protocol."""

import json
import logging
import threading
from typing import Callable, Optional

import serial

from config import SERIAL_BAUD, SERIAL_PORT

logger = logging.getLogger(__name__)


class ArduinoComm:
    def __init__(self):
        self._ser: Optional[serial.Serial] = None
        self._lock = threading.Lock()
        self._on_button: Optional[Callable] = None
        self._on_door_alert: Optional[Callable] = None
        self._reader_thread: Optional[threading.Thread] = None
        self._running = False

    def start(self, on_button: Callable, on_door_alert: Callable):
        self._on_button = on_button
        self._on_door_alert = on_door_alert
        try:
            self._ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=1)
            self._running = True
            self._reader_thread = threading.Thread(target=self._read_loop, daemon=True)
            self._reader_thread.start()
            logger.info("Arduino serial connected on %s", SERIAL_PORT)
        except serial.SerialException as e:
            logger.error("Failed to open serial port: %s", e)

    def stop(self):
        self._running = False
        if self._ser:
            self._ser.close()

    def _send(self, payload: dict):
        if not self._ser or not self._ser.is_open:
            return
        with self._lock:
            line = json.dumps(payload) + "\n"
            self._ser.write(line.encode())

    def _read_loop(self):
        while self._running:
            try:
                raw = self._ser.readline().decode(errors="ignore").strip()
                if not raw:
                    continue
                msg = json.loads(raw)
                event = msg.get("event")
                if event == "button_press" and self._on_button:
                    self._on_button()
                elif event == "door_alert" and self._on_door_alert:
                    self._on_door_alert()
            except json.JSONDecodeError:
                pass
            except Exception as e:
                logger.error("Serial read error: %s", e)

    # ── Commands to Arduino ────────────────────────────────────────────────────

    def led_on(self):
        self._send({"cmd": "led_on"})

    def led_off(self):
        self._send({"cmd": "led_off"})

    def buzzer(self, times: int):
        self._send({"cmd": "buzzer", "times": times})

    def oled(self, line1: str, line2: str = "", line3: str = ""):
        self._send({"cmd": "oled", "l1": line1[:21], "l2": line2[:21], "l3": line3[:21]})

    def oled_clear(self):
        self._send({"cmd": "oled_clear"})
