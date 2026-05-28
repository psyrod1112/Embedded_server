/*
 * FridgeFIFO — Arduino firmware
 *
 * Handles: LED, buzzer, push button, OLED display
 * Communicates with Raspberry Pi over Serial (JSON Lines, 115200 baud)
 *
 * Libraries required (install via Arduino Library Manager):
 *   - ArduinoJson  (Benoit Blanchon) >= 6.x
 *   - Adafruit SSD1306
 *   - Adafruit GFX Library
 */

#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

// ── OLED ──────────────────────────────────────────────────────────
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ── State ─────────────────────────────────────────────────────────
bool     ledState       = false;
int      lastButtonRead = HIGH;
uint32_t lastDebounce   = 0;

// ── Setup ─────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    pinMode(LED_PIN,    OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    digitalWrite(LED_PIN,    LOW);
    digitalWrite(BUZZER_PIN, LOW);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        // OLED init failed — continue without display
        while (true) {}
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("FridgeFIFO");
    display.println("Booting...");
    display.display();
}

// ── Helpers ───────────────────────────────────────────────────────
void doBuzzer(int times) {
    for (int i = 0; i < times; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(BUZZER_BEEP_MS);
        digitalWrite(BUZZER_PIN, LOW);
        if (i < times - 1) delay(BUZZER_GAP_MS);
    }
}

void showOled(const char* l1, const char* l2, const char* l3) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);  display.println(l1);
    display.setCursor(0, 22); display.println(l2);
    display.setCursor(0, 44); display.println(l3);
    display.display();
}

void sendEvent(const char* event) {
    StaticJsonDocument<64> doc;
    doc["event"] = event;
    serializeJson(doc, Serial);
    Serial.println();
}

// ── Command Parser ────────────────────────────────────────────────
void handleCommand(JsonDocument& doc) {
    const char* cmd = doc["cmd"];
    if (!cmd) return;

    if (strcmp(cmd, "led_on") == 0) {
        digitalWrite(LED_PIN, HIGH);
        ledState = true;

    } else if (strcmp(cmd, "led_off") == 0) {
        digitalWrite(LED_PIN, LOW);
        ledState = false;

    } else if (strcmp(cmd, "buzzer") == 0) {
        int times = doc["times"] | 1;
        doBuzzer(times);

    } else if (strcmp(cmd, "oled") == 0) {
        const char* l1 = doc["l1"] | "";
        const char* l2 = doc["l2"] | "";
        const char* l3 = doc["l3"] | "";
        showOled(l1, l2, l3);

    } else if (strcmp(cmd, "oled_clear") == 0) {
        display.clearDisplay();
        display.display();
    }
}

// ── Loop ──────────────────────────────────────────────────────────
void loop() {
    // ── Read button with debounce ──────────────────────────────────
    int reading = digitalRead(BUTTON_PIN);
    if (reading != lastButtonRead) {
        lastDebounce = millis();
    }
    if ((millis() - lastDebounce) > BUTTON_DEBOUNCE) {
        // Button is LOW when pressed (INPUT_PULLUP)
        if (reading == LOW) {
            sendEvent("button_press");
            // wait for release to avoid repeat
            while (digitalRead(BUTTON_PIN) == LOW) delay(10);
        }
    }
    lastButtonRead = reading;

    // ── Read serial commands from RPi ─────────────────────────────
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) return;

        StaticJsonDocument<256> doc;
        DeserializationError err = deserializeJson(doc, line);
        if (!err) {
            handleCommand(doc);
        }
    }
}
