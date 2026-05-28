#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#define OLED_ADDR 0x78 // 0x3C << 1

// --- [HX711 로드셀 핀 정의] ---
#define HX711_DOUT_PIN  PD6  // 아두이노 D2 (입력)
#define HX711_SCK_PIN   PD5  // 아두이노 D3 (출력)

extern const uint8_t font[] PROGMEM; 

// --- [순수 AVR 방식 I2C 함수] ---
void i2c_init(void) {
    TWBR = 72; 
    TWSR = 0;   
}

void i2c_start(void) {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

void i2c_stop(void) {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    while (TWCR & (1 << TWSTO));
}

void i2c_write(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

void oled_cmd(uint8_t cmd) {
    i2c_start(); i2c_write(OLED_ADDR); i2c_write(0x00); i2c_write(cmd); i2c_stop();
}

void oled_data(uint8_t data) {
    i2c_start(); i2c_write(OLED_ADDR); i2c_write(0x40); i2c_write(data); i2c_stop();
}

void oled_init(void) {
    _delay_ms(100);
    oled_cmd(0xAE); oled_cmd(0xD5); oled_cmd(0x80); oled_cmd(0xA8); oled_cmd(0x3F);
    oled_cmd(0xD3); oled_cmd(0x00); oled_cmd(0x40); oled_cmd(0x8D); oled_cmd(0x14); 
    oled_cmd(0x20); oled_cmd(0x02); oled_cmd(0xA1); oled_cmd(0xC8); oled_cmd(0xDA); 
    oled_cmd(0x12); oled_cmd(0x81); oled_cmd(0xCF); oled_cmd(0xD9); oled_cmd(0xF1);
    oled_cmd(0xDB); oled_cmd(0x40); oled_cmd(0xA4); oled_cmd(0xA6); oled_cmd(0xAF); 
}

void oled_clear(void) {
    for (uint8_t page = 0; page < 8; page++) {
        oled_cmd(0xB0 + page); oled_cmd(0x00); oled_cmd(0x10);
        for (uint16_t i = 0; i < 128; i++) oled_data(0x00);
    }
}

void oled_putchar(uint8_t page, uint8_t col, char c) {
    if (c < 32 || c > 127) return; 
    oled_cmd(0xB0 + page); oled_cmd(col & 0x0F); oled_cmd(0x10 | (col >> 4)); 
    uint16_t font_idx = (c - 32) * 5;
    for (uint8_t i = 0; i < 5; i++) oled_data(pgm_read_byte(&(font[font_idx + i])));
    oled_data(0x00); 
}

void oled_print(uint8_t page, uint8_t col, const char* str) {
    while (*str) { oled_putchar(page, col, *str++); col += 6; if (col > 122) break; }
}

// --- [순수 AVR 방식 HX711 로드셀 제어 함수] ---
void hx711_init(void) {
    DDRD &= ~(1 << HX711_DOUT_PIN); // DOUT 핀 입력 설정
    DDRD |= (1 << HX711_SCK_PIN);   // SCK 핀 출력 설정
    PORTD &= ~(1 << HX711_SCK_PIN); // SCK 로우(Low) 상태로 초기화 (전원 켜기)
}

// 로드셀로부터 24비트 원시 데이터 읽기
long hx711_read(void) {
    long count = 0;

    // DOUT 핀이 Low가 될 때까지 대기 (데이터 준비 완료 신호)
    while (PIND & (1 << HX711_DOUT_PIN));

    // 24비트 데이터 비트 시프트로 읽어오기
    for (uint8_t i = 0; i < 24; i++) {
        PORTD |= (1 << HX711_SCK_PIN);  // 클럭 HIGH
        _delay_us(1);
        count = count << 1;
        PORTD &= ~(1 << HX711_SCK_PIN); // 클럭 LOW
        _delay_us(1);
        
        if (PIND & (1 << HX711_DOUT_PIN)) {
            count++;
        }
    }

    // 25번째 클럭을 주어 다음 채널/게인(128) 설정
    PORTD |= (1 << HX711_SCK_PIN);
    _delay_us(1);
    PORTD &= ~(1 << HX711_SCK_PIN);
    _delay_us(1);

    // 24비트 음수 처리 (2의 보수 확장)
    if (count & 0x800000) {
        count |= 0xFF000000;
    }

    return count;
}

// --- [메인 로직] ---
int main(void) {
    i2c_init();  
    oled_init(); 
    oled_clear(); 
    hx711_init(); // 로드셀 초기화

    oled_print(0, 0, "=== LOADCELL ===");
    oled_print(2, 0, "Initializing...");

    _delay_ms(100);
    oled_clear();

    while (1) {
        long raw_weight = hx711_read(); // 무게 원시 데이터 읽기

        oled_print(0, 0, "=== LOADCELL ===");
        
        // 원시 데이터 값 조건에 맞춰 간단히 화면 피드백 주기 테스트
        if (raw_weight > 100000) { // 예시 문구 (실제 로드셀 눌리는 값에 맞게 변경 가능)
            oled_print(3, 0, "Status: LOADED   ");
        } else {
            oled_print(3, 0, "Status: EMPTY    ");
        }
        
        _delay_ms(200); // 디스플레이 갱신 주기 대기
    }
    return 0;
}
