#pragma once

// ── Pin Definitions ───────────────────────────────────────────────
#define LED_PIN      4    // Red LED
#define BUZZER_PIN   5    // Active buzzer
#define BUTTON_PIN   6    // Push button (INPUT_PULLUP — LOW when pressed)

// OLED via I2C: SDA=A4, SCL=A5 (Uno) — no defines needed, Wire uses them

// ── OLED ──────────────────────────────────────────────────────────
#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64

// ── Timing ────────────────────────────────────────────────────────
#define BUZZER_BEEP_MS   150   // duration of one beep
#define BUZZER_GAP_MS    100   // gap between beeps
#define BUTTON_DEBOUNCE  50    // ms
