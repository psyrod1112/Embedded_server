#pragma once

// ── Pin Definitions ───────────────────────────────────────────────
#define LED_PIN      13   // Red LED
#define BUTTON_PIN   1    // Push button (INPUT_PULLUP — LOW when pressed)
                          // NOTE: pin 1 = TX; avoid using Serial while button is wired here

#define TRIG_PIN     7    // Ultrasonic sensor Trig
#define ECHO_PIN     8    // Ultrasonic sensor Echo

// OLED via I2C: SDA=A4, SCL=A5 (Uno) — no defines needed, Wire uses them

// ── OLED ──────────────────────────────────────────────────────────
#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64

// ── Timing ────────────────────────────────────────────────────────
#define BUTTON_DEBOUNCE     50    // ms
#define ULTRASONIC_TIMEOUT  30000 // us — ~5 m max range
