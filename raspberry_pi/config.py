import os
from dotenv import load_dotenv

load_dotenv()

# AWS server base URL
SERVER_URL = os.getenv("SERVER_URL", "http://your-aws-server.com")
API_TOKEN  = os.getenv("API_TOKEN", "")          # JWT — set after first login

# Serial port to Arduino
SERIAL_PORT = os.getenv("SERIAL_PORT", "/dev/ttyUSB0")
SERIAL_BAUD = 115200

# HX711 GPIO pins (BCM numbering)
HX711_DOUT = 5
HX711_SCK  = 6

# HC-SR04 ultrasonic sensor GPIO pins
ULTRASONIC_TRIG = 23
ULTRASONIC_ECHO = 24

# Thresholds
WEIGHT_NOISE_THRESHOLD = 10.0    # grams — smaller deltas are noise
DOOR_OPEN_DISTANCE_CM  = 15.0   # cm — closer = door open
DOOR_OPEN_ALERT_SEC    = 60     # seconds — alert after this long open
SCAN_TIMEOUT_SEC       = 30     # seconds — reset state if door not opened
MAX_SCAN_RETRIES       = 5      # consecutive scan failures before error mode
