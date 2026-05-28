// main.cpp / FridgeFIFO AVR version

#define F_CPU 16000000UL

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_ADDRESS   0x3C

// Arduino Uno 기준
#define LED_DDR     DDRD
#define LED_PORT    PORTD
#define LED_BIT     PD5

#define BUZZER_DDR  DDRD
#define BUZZER_PORT PORTD
#define BUZZER_BIT  PD6

#define BUTTON_DDR  DDRD
#define BUTTON_PORT PORTD
#define BUTTON_PINR PIND
#define BUTTON_BIT  PD2

#define BUTTON_DEBOUNCE_MS 50
#define BUZZER_BEEP_MS     100
#define BUZZER_GAP_MS      80

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, NULL, -1);

static uint32_t ms_counter = 0;

ISR(TIMER0_COMPA_vect) {
    ms_counter++;
}

uint32_t millis_avr() {
    uint32_t m;
    cli();
    m = ms_counter;
    sei();
    return m;
}

void timer0_millis_init() {
    TCCR0A = (1 << WGM01);
    OCR0A = 249;                 // 16MHz / 64 / 250 = 1kHz
    TIMSK0 = (1 << OCIE0A);
    TCCR0B = (1 << CS01) | (1 << CS00);
    sei();
}

void uart_init() {
    uint16_t ubrr = 8;           // 115200 baud, 16MHz, U2X0=1
    UCSR0A = (1 << U2X0);
    UBRR0H = ubrr >> 8;
    UBRR0L = ubrr;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_tx(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

void uart_print(const char *s) {
    while (*s) uart_tx(*s++);
}

uint8_t uart_available() {
    return (UCSR0A & (1 << RXC0));
}

char uart_rx() {
    while (!uart_available());
    return UDR0;
}

void oled_write_str(const char *s) {
    while (*s) display.write((uint8_t)*s++);
}

void showOled(const char *l1, const char *l2, const char *l3) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    oled_write_str(l1);

    display.setCursor(0, 22);
    oled_write_str(l2);

    display.setCursor(0, 44);
    oled_write_str(l3);

    display.display();
}

void doBuzzer(uint8_t times) {
    for (uint8_t i = 0; i < times; i++) {
        BUZZER_PORT |= (1 << BUZZER_BIT);
        _delay_ms(BUZZER_BEEP_MS);
        BUZZER_PORT &= ~(1 << BUZZER_BIT);
        if (i < times - 1) _delay_ms(BUZZER_GAP_MS);
    }
}

void sendEvent(const char *event) {
    uart_print("{\"event\":\"");
    uart_print(event);
    uart_print("\"}\n");
}

int json_get_string(const char *json, const char *key, char *out, uint8_t maxLen) {
    char pattern[16];
    strcpy(pattern, "\"");
    strcat(pattern, key);
    strcat(pattern, "\":\"");

    char *p = strstr((char *)json, pattern);
    if (!p) return 0;

    p += strlen(pattern);
    uint8_t i = 0;

    while (*p && *p != '"' && i < maxLen - 1) {
        out[i++] = *p++;
    }

    out[i] = '\0';
    return 1;
}

int json_get_int(const char *json, const char *key, int defaultVal) {
    char pattern[16];
    strcpy(pattern, "\"");
    strcat(pattern, key);
    strcat(pattern, "\":");

    char *p = strstr((char *)json, pattern);
    if (!p) return defaultVal;

    p += strlen(pattern);
    return atoi(p);
}

void handleCommand(char *line) {
    char cmd[20];

    if (!json_get_string(line, "cmd", cmd, sizeof(cmd))) return;

    if (strcmp(cmd, "led_on") == 0) {
        LED_PORT |= (1 << LED_BIT);

    } else if (strcmp(cmd, "led_off") == 0) {
        LED_PORT &= ~(1 << LED_BIT);

    } else if (strcmp(cmd, "buzzer") == 0) {
        int times = json_get_int(line, "times", 1);
        if (times < 1) times = 1;
        if (times > 10) times = 10;
        doBuzzer((uint8_t)times);

    } else if (strcmp(cmd, "oled") == 0) {
        char l1[22], l2[22], l3[22];

        json_get_string(line, "l1", l1, sizeof(l1));
        json_get_string(line, "l2", l2, sizeof(l2));
        json_get_string(line, "l3", l3, sizeof(l3));

        showOled(l1, l2, l3);

    } else if (strcmp(cmd, "oled_clear") == 0) {
        display.clearDisplay();
        display.display();
    }
}

void setup_avr() {
    LED_DDR |= (1 << LED_BIT);
    BUZZER_DDR |= (1 << BUZZER_BIT);

    BUTTON_DDR &= ~(1 << BUTTON_BIT);
    BUTTON_PORT |= (1 << BUTTON_BIT);   // pull-up

    LED_PORT &= ~(1 << LED_BIT);
    BUZZER_PORT &= ~(1 << BUZZER_BIT);

    uart_init();
    timer0_millis_init();

    if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        showOled("FridgeFIFO", "Booting...", "");
    }
}

int main(void) {
    setup_avr();

    char line[256];
    uint8_t idx = 0;

    uint8_t lastButtonRead = 1;
    uint32_t lastDebounce = 0;

    while (1) {
        uint8_t reading = (BUTTON_PINR & (1 << BUTTON_BIT)) ? 1 : 0;

        if (reading != lastButtonRead) {
            lastDebounce = millis_avr();
        }

        if ((millis_avr() - lastDebounce) > BUTTON_DEBOUNCE_MS) {
            if (reading == 0) {
                sendEvent("button_press");
                while (!(BUTTON_PINR & (1 << BUTTON_BIT))) {
                    _delay_ms(10);
                }
            }
        }

        lastButtonRead = reading;

        if (uart_available()) {
            char c = uart_rx();

            if (c == '\n') {
                line[idx] = '\0';
                handleCommand(line);
                idx = 0;
            } else if (c != '\r' && idx < sizeof(line) - 1) {
                line[idx++] = c;
            }
        }
    }
}