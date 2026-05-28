"""
State machine for the FIFO fridge controller.

States:
  IDLE           — waiting for expiry date scan
  SCANNED        — scan succeeded, LED on, 30 s timeout, waiting for switch press
  ERROR_RECOVERY — wrong weight direction detected; waiting for user to correct

Switch press (from Arduino) is the universal trigger:
  - Read current weight
  - delta = current - weight_before
  - delta > +threshold  →  stock-in path
  - delta < -threshold  →  stock-out path  (or error if state==SCANNED)
  - |delta| < threshold →  noise, ignore
"""

import logging
import threading
import time
from datetime import date
from enum import Enum, auto
from typing import Optional

from api_client import scan_image, stock_in, stock_out
from camera import Camera
from config import MAX_SCAN_RETRIES, SCAN_TIMEOUT_SEC, WEIGHT_NOISE_THRESHOLD
from serial_comm import ArduinoComm
from weight_sensor import WeightSensor

logger = logging.getLogger(__name__)


class State(Enum):
    IDLE = auto()
    SCANNED = auto()
    ERROR_RECOVERY = auto()


class FridgeFSM:
    def __init__(self, arduino: ArduinoComm, weight: WeightSensor, camera: Camera):
        self._arduino = arduino
        self._weight = weight
        self._camera = camera

        self._state = State.IDLE
        self._weight_before: float = 0.0
        self._pending_expiry: Optional[date] = None
        self._scan_retries = 0
        self._timeout_timer: Optional[threading.Timer] = None
        self._lock = threading.Lock()

    # ── Public callbacks (wired in main.py) ───────────────────────────────────

    def on_button_press(self):
        """Called from serial reader thread."""
        with self._lock:
            self._handle_button()

    def on_door_open_long(self):
        """Called when door has been open > DOOR_OPEN_ALERT_SEC."""
        self._arduino.buzzer(3)
        self._arduino.oled("문이 열려있습니다", "Close Door")

    def trigger_scan(self):
        """Called from main scan loop when camera captures a frame."""
        with self._lock:
            if self._state != State.IDLE:
                return
            self._do_scan()

    # ── Internal ──────────────────────────────────────────────────────────────

    def _do_scan(self):
        jpeg = self._camera.capture_jpeg()
        result = scan_image(jpeg)

        if result.get("success"):
            self._pending_expiry = date.fromisoformat(result["expiry_date"])
            self._weight_before = self._weight.read_grams()
            self._scan_retries = 0
            self._transition_to_scanned()
        else:
            self._scan_retries += 1
            self._arduino.buzzer(1)
            logger.info("Scan failed (%d/%d): %s", self._scan_retries, MAX_SCAN_RETRIES, result.get("error"))

            if self._scan_retries >= MAX_SCAN_RETRIES:
                self._arduino.buzzer(2)
                self._arduino.oled("스캔 반복 실패", "수동 입력 필요", "앱을 확인하세요")
                self._scan_retries = 0

    def _transition_to_scanned(self):
        self._state = State.SCANNED
        self._arduino.led_on()
        self._arduino.oled(
            "스캔 성공",
            f"유통기한: {self._pending_expiry}",
            "물건 넣고 버튼 누르세요",
        )
        self._start_timeout()
        logger.info("State → SCANNED  expiry=%s", self._pending_expiry)

    def _start_timeout(self):
        self._cancel_timeout()
        self._timeout_timer = threading.Timer(SCAN_TIMEOUT_SEC, self._on_timeout)
        self._timeout_timer.daemon = True
        self._timeout_timer.start()

    def _cancel_timeout(self):
        if self._timeout_timer:
            self._timeout_timer.cancel()
            self._timeout_timer = None

    def _on_timeout(self):
        with self._lock:
            if self._state != State.SCANNED:
                return
            logger.info("Scan timeout — resetting to IDLE")
            self._reset_to_idle()
            self._arduino.oled("시간 초과", "다시 스캔하세요")

    def _reset_to_idle(self):
        self._cancel_timeout()
        self._state = State.IDLE
        self._pending_expiry = None
        self._weight_before = 0.0
        self._arduino.led_off()

    def _handle_button(self):
        current_weight = self._weight.read_grams()
        delta = current_weight - self._weight_before

        if abs(delta) < WEIGHT_NOISE_THRESHOLD:
            logger.debug("Button press — delta %.1fg below threshold, ignoring", delta)
            return

        if self._state == State.IDLE:
            self._handle_idle_button(delta, current_weight)
        elif self._state == State.SCANNED:
            self._handle_scanned_button(delta, current_weight)
        elif self._state == State.ERROR_RECOVERY:
            self._handle_error_recovery_button(delta)

    def _handle_idle_button(self, delta: float, current_weight: float):
        if delta < -WEIGHT_NOISE_THRESHOLD:
            # Stock-out
            self._weight_before = current_weight + abs(delta)  # restore for next op
            self._process_stock_out(abs(delta))
        else:
            # Weight increased without scanning — error
            self._arduino.buzzer(2)
            self._arduino.oled("오류: 스캔 먼저", "유통기한 스캔 후", "버튼 누르세요")
            self._weight_before = current_weight
            self._state = State.ERROR_RECOVERY
            logger.warning("Button in IDLE with weight increase — error recovery")

    def _handle_scanned_button(self, delta: float, current_weight: float):
        if delta > WEIGHT_NOISE_THRESHOLD:
            # Stock-in
            self._cancel_timeout()
            self._process_stock_in(delta)
        else:
            # Weight decreased while in SCANNED — user took something out by mistake
            self._arduino.buzzer(2)
            self._arduino.oled("오류: 무게 감소", "물건을 다시 넣고", "버튼 누르세요")
            self._state = State.ERROR_RECOVERY
            logger.warning("Weight decreased during SCANNED — error recovery")

    def _handle_error_recovery_button(self, delta: float):
        # Check if weight returned close to weight_before
        if abs(delta) < WEIGHT_NOISE_THRESHOLD:
            self._arduino.oled("해결됨", "다시 시도하세요")
            if self._pending_expiry:
                # Was in SCANNED — go back to SCANNED
                self._transition_to_scanned()
            else:
                self._reset_to_idle()
        else:
            self._arduino.buzzer(1)
            self._arduino.oled("아직 오류", "원래 무게로 복원", "후 버튼 누르세요")

    def _process_stock_in(self, delta: float):
        self._arduino.oled("입고 처리중...", "잠시 기다려주세요")
        result = stock_in(self._pending_expiry, delta)

        if result:
            name = result["type_name"]
            qty = result["quantity"]
            pos = result["fifo_position"]
            pos_end = result["fifo_position_end"]
            expiry = result["expiry_date"]

            pos_str = f"{pos}번째" if pos == pos_end else f"{pos}~{pos_end}번째"
            self._arduino.oled(name, f"유통기한: {expiry}", pos_str)
            logger.info("Stock-in OK: %s qty=%d pos=%s", name, qty, pos_str)
        else:
            self._arduino.oled("서버 오류", "앱에서 확인하세요")

        self._reset_to_idle()

    def _process_stock_out(self, delta: float):
        self._arduino.oled("출고 처리중...", "잠시 기다려주세요")
        result = stock_out(delta)

        if result:
            if result.get("auto_resolved"):
                removed = result.get("removed_item", {})
                self._arduino.oled("출고 성공", removed.get("type_name", ""), "")
            elif result.get("error"):
                self._arduino.oled("매칭 없음", "앱에서 수동처리", "필요합니다")
            else:
                self._arduino.oled("후보 선택", "앱에서 확인", "해주세요")
        else:
            self._arduino.oled("서버 오류", "앱에서 확인하세요")

        logger.info("Stock-out processed  delta=%.1fg", delta)
