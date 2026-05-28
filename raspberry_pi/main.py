"""Raspberry Pi entry point."""

import logging
import signal
import sys
import time

from camera import Camera
from serial_comm import ArduinoComm
from state_machine import FridgeFSM
from ultrasonic import UltrasonicSensor
from weight_sensor import WeightSensor

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
)
logger = logging.getLogger(__name__)

SCAN_INTERVAL = 2.0  # seconds between camera frames in IDLE state


def main():
    logger.info("FridgeFIFO starting...")

    camera   = Camera()
    weight   = WeightSensor()
    arduino  = ArduinoComm()
    sonic    = UltrasonicSensor()
    fsm      = FridgeFSM(arduino, weight, camera)

    arduino.start(
        on_button=fsm.on_button_press,
        on_door_alert=fsm.on_door_open_long,
    )
    sonic.start(on_door_open_long=fsm.on_door_open_long)

    arduino.oled("FridgeFIFO", "준비 완료", "스캔을 시작하세요")
    logger.info("System ready")

    def shutdown(sig, frame):
        logger.info("Shutting down...")
        sonic.stop()
        arduino.stop()
        camera.close()
        sys.exit(0)

    signal.signal(signal.SIGINT, shutdown)
    signal.signal(signal.SIGTERM, shutdown)

    # Main loop — periodically trigger camera scan when in IDLE state
    while True:
        fsm.trigger_scan()
        time.sleep(SCAN_INTERVAL)


if __name__ == "__main__":
    main()
