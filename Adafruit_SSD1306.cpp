#include <avr/io.h>
#include <avr/pgmspace.h>  // 👈 플래시 메모리 매크로 헤더 안착
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>

#include "Adafruit_SSD1306.h"
#include "splash.h"

#define ARDUINO 0

// -------------------------------------------------------------
// [PROGMEM 명령어 배열들을 함수 외부 전역으로 격리 (에러 해결)]
// -------------------------------------------------------------
static const uint8_t PROGMEM initcmd[] = {
    SSD1306_DISPLAYOFF,
    SSD1306_SETDISPLAYCLOCKDIV, 0x80,
    SSD1306_SETMULTIPLEX, 0x1F, // 기본 32 높이 기준으로 안전 셋업 (begin에서 마스킹)
    SSD1306_SETDISPLAYOFFSET, 0x00,
    SSD1306_SETSTARTLINE | 0x00,
    SSD1306_CHARGEPUMP, 0x14,
    SSD1306_MEMORYMODE, 0x00, 
    SSD1306_SEGREMAP | 0x01,
    SSD1306_COMSCANDEC,
    SSD1306_SETCOMPINS, 0x02,
    SSD1306_SETCONTRAST, 0xCF,
    SSD1306_SETPRECHARGE, 0xF1,
    SSD1306_SETVCOMDETECT, 0x40,
    SSD1306_DISPLAYALLON_RESUME,
    SSD1306_NORMALDISPLAY,
    SSD1306_DEACTIVATE_SCROLL,
    SSD1306_DISPLAYON
};

static const uint8_t PROGMEM scroll_rt_cmd[] = {
    SSD1306_RIGHT_HORIZONTAL_SCROLL, 0x00, 0, 0x00, 0, 0x00, 0xFF, SSD1306_ACTIVATE_SCROLL
};

static const uint8_t PROGMEM scroll_lt_cmd[] = {
    SSD1306_LEFT_HORIZONTAL_SCROLL, 0x00, 0, 0x00, 0, 0x00, 0xFF, SSD1306_ACTIVATE_SCROLL
};

// -------------------------------------------------------------
// [순수 AVR 레지스터 기반 I2C(TWI) 제어 함수]
// -------------------------------------------------------------
static void AVR_I2C_Init(void) {
    TWBR = 12;
    TWSR = 0x00; 
}

static void AVR_I2C_Start(void) {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

static void AVR_I2C_Stop(void) {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    _delay_us(5);
}

static void AVR_I2C_Write(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

static void AVR_I2C_BeginTransmission(uint8_t address) {
    AVR_I2C_Start();
    AVR_I2C_Write(address << 1); 
}

// -------------------------------------------------------------
// [Adafruit_SSD1306 생성자 수동 매칭]
// -------------------------------------------------------------
Adafruit_SSD1306::Adafruit_SSD1306(uint8_t w, uint8_t h, void *twi,
                                   int8_t rst_pin, uint32_t clkDuring,
                                   uint32_t clkAfter)
    : Adafruit_GFX(w, h), spi(NULL), wire(NULL), buffer(NULL), i2caddr(0x3C),
      vccstate(0), rstPin(rst_pin) {
}

Adafruit_SSD1306::Adafruit_SSD1306(uint8_t w, uint8_t h, int8_t mosi_pin,
                                   int8_t sclk_pin, int8_t dc_pin,
                                   int8_t rst_pin, int8_t cs_pin)
    : Adafruit_GFX(w, h), spi(NULL), wire(NULL), buffer(NULL), i2caddr(0),
      vccstate(0), rstPin(rst_pin) {
}

Adafruit_SSD1306::Adafruit_SSD1306(uint8_t w, uint8_t h, void *spi_ptr,
                                   int8_t dc_pin, int8_t rst_pin, int8_t cs_pin,
                                   uint32_t bitrate)
    : Adafruit_GFX(w, h), spi(NULL), wire(NULL), buffer(NULL), i2caddr(0),
      vccstate(0), rstPin(rst_pin) {
}

Adafruit_SSD1306::Adafruit_SSD1306(int8_t mosi_pin, int8_t sclk_pin,
                                   int8_t dc_pin, int8_t rst_pin, int8_t cs_pin)
    : Adafruit_GFX(128, 32), spi(NULL), wire(NULL), buffer(NULL), i2caddr(0),
      vccstate(0), rstPin(rst_pin) {
}

Adafruit_SSD1306::Adafruit_SSD1306(int8_t dc_pin, int8_t rst_pin, int8_t cs_pin)
    : Adafruit_GFX(128, 32), spi(NULL), wire(NULL), buffer(NULL), i2caddr(0),
      vccstate(0), rstPin(rst_pin) {
}

Adafruit_SSD1306::Adafruit_SSD1306(int8_t rst_pin)
    : Adafruit_GFX(128, 32), spi(NULL), wire(NULL), buffer(NULL), i2caddr(0),
      vccstate(0), rstPin(rst_pin) {
}

Adafruit_SSD1306::~Adafruit_SSD1306(void) {
    if (buffer) {
        free(buffer);
        buffer = NULL;
    }
}

// -------------------------------------------------------------
// [핵심 제어 함수 매커니즘 구현]
// -------------------------------------------------------------
void Adafruit_SSD1306::ssd1306_command1(uint8_t c) {
    AVR_I2C_BeginTransmission(i2caddr);
    AVR_I2C_Write(0x00); 
    AVR_I2C_Write(c);
    AVR_I2C_Stop();
}

void Adafruit_SSD1306::ssd1306_commandList(const uint8_t *c, uint8_t n) {
    AVR_I2C_BeginTransmission(i2caddr);
    AVR_I2C_Write(0x00); 
    while (n--) {
        AVR_I2C_Write(pgm_read_byte(c++));
    }
    AVR_I2C_Stop();
}

void Adafruit_SSD1306::ssd1306_command(uint8_t c) {
    ssd1306_command1(c);
}

bool Adafruit_SSD1306::begin(uint8_t switchvcc, uint8_t vccarg, bool reset, bool periphBegin) {
    if (buffer) {
        free(buffer);
        buffer = NULL;
    }
    
    uint16_t buffer_size = WIDTH * ((HEIGHT + 7) / 8);
    if (!(buffer = (uint8_t *)malloc(buffer_size))) {
        return false;
    }
    
    clearDisplay();
    vccstate = switchvcc;
    i2caddr = vccarg ? vccarg : 0x3C; 

    AVR_I2C_Init();

    // 외부로 격리된 공용 initcmd 배열 전송
    ssd1306_commandList(initcmd, sizeof(initcmd));
    
    // 화면 높이에 따른 유동 커맨드 추가 보정 전달
    ssd1306_command1(SSD1306_SETMULTIPLEX);
    ssd1306_command1(HEIGHT - 1);
    ssd1306_command1(SSD1306_SETCOMPINS);
    ssd1306_command1(HEIGHT == 64 ? 0x12 : 0x02);

    return true;
}

void Adafruit_SSD1306::display(void) {
    if (!buffer) return;

    uint16_t buffer_size = WIDTH * ((HEIGHT + 7) / 8);
    
    // dscmd 동적 에러 소지를 제거하기 위해 command1으로 개별 분해 발송
    ssd1306_command1(SSD1306_COLUMNADDR);
    ssd1306_command1(0);
    ssd1306_command1(WIDTH - 1);
    ssd1306_command1(SSD1306_PAGEADDR);
    ssd1306_command1(0);
    ssd1306_command1((HEIGHT / 8) - 1);

    uint8_t *ptr = buffer;
    uint16_t count = buffer_size;

    AVR_I2C_BeginTransmission(i2caddr);
    AVR_I2C_Write(0x40); 
    
    while (count--) {
        AVR_I2C_Write(*ptr++);
    }
    AVR_I2C_Stop();
}

void Adafruit_SSD1306::clearDisplay(void) {
    if (buffer) {
        uint16_t buffer_size = WIDTH * ((HEIGHT + 7) / 8);
        memset(buffer, 0, buffer_size);
    }
}

void Adafruit_SSD1306::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if ((x < 0) || (x >= width()) || (y < 0) || (y >= height())) return;

    switch (getRotation()) {
    case 1:
        asm("swap %0" : "=r"(x) : "0"(x)); 
        x = WIDTH - x - 1;
        break;
    case 2:
        x = WIDTH - x - 1;
        y = HEIGHT - y - 1;
        break;
    case 3:
        asm("swap %0" : "=r"(x) : "0"(x));
        y = HEIGHT - y - 1;
        break;
    }

    if (color) {
        buffer[x + (y / 8) * WIDTH] |= (1 << (y & 7));
    } else {
        buffer[x + (y / 8) * WIDTH] &= ~(1 << (y & 7));
    }
}

void Adafruit_SSD1306::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    drawFastHLineInternal(x, y, w, color);
}

void Adafruit_SSD1306::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    drawFastVLineInternal(x, y, h, color);
}

void Adafruit_SSD1306::drawFastHLineInternal(int16_t x, int16_t y, int16_t w, uint16_t color) {
    if ((y >= 0) && (y < HEIGHT)) {
        if (x < 0) { w += x; x = 0; }
        if ((x + w) > WIDTH) w = WIDTH - x;
        if (w > 0) {
            uint8_t *pBuf = &buffer[(y / 8) * WIDTH + x];
            uint8_t mask = 1 << (y & 7);
            if (color) {
                while (w--) *pBuf++ |= mask;
            } else {
                while (w--) *pBuf++ &= ~mask;
            }
        }
    }
}

void Adafruit_SSD1306::drawFastVLineInternal(int16_t x, int16_t y, int16_t h, uint16_t color) {
    if ((x >= 0) && (x < WIDTH)) {
        if (y < 0) { h += y; y = 0; }
        if ((y + h) > HEIGHT) h = HEIGHT - y;
        if (h > 0) {
            uint8_t *pBuf = &buffer[(y / 8) * WIDTH + x];
            uint8_t mod = y & 7;
            if (mod) {
                mod = 8 - mod;
                uint8_t mask = ~(0xFF >> mod);
                if (h < mod) mask &= (0xFF >> (mod - h));
                if (color) *pBuf |= mask; else *pBuf &= ~mask;
                if (h < mod) return;
                h -= mod;
                pBuf += WIDTH;
            }
            if (h >= 8) {
                uint8_t val = color ? 0xFF : 0x00;
                do {
                    *pBuf = val;
                    pBuf += WIDTH;
                    h -= 8;
                } while (h >= 8);
            }
            if (h) {
                uint8_t mask = 0xFF >> (8 - h);
                if (color) *pBuf |= mask; else *pBuf &= ~mask;
            }
        }
    }
}

void Adafruit_SSD1306::invertDisplay(bool i) {
    ssd1306_command(i ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY);
}

void Adafruit_SSD1306::dim(bool dim) {
    ssd1306_command(SSD1306_SETCONTRAST);
    ssd1306_command(dim ? 0 : contrast);
}

void Adafruit_SSD1306::startscrollright(uint8_t start, uint8_t stop) {
    ssd1306_commandList(scroll_rt_cmd, sizeof(scroll_rt_cmd));
}

void Adafruit_SSD1306::startscrollleft(uint8_t start, uint8_t stop) {
    ssd1306_commandList(scroll_lt_cmd, sizeof(scroll_lt_cmd));
}

void Adafruit_SSD1306::startscrolldiagright(uint8_t start, uint8_t stop) {
    ssd1306_command1(SSD1306_SET_VERTICAL_SCROLL_AREA);
    ssd1306_command1(0x00);
    ssd1306_command1(HEIGHT);
    ssd1306_commandList(scroll_rt_cmd, sizeof(scroll_rt_cmd));
}

void Adafruit_SSD1306::startscrolldiagleft(uint8_t start, uint8_t stop) {
    ssd1306_command1(SSD1306_SET_VERTICAL_SCROLL_AREA);
    ssd1306_command1(0x00);
    ssd1306_command1(HEIGHT);
    ssd1306_commandList(scroll_lt_cmd, sizeof(scroll_lt_cmd));
}

void Adafruit_SSD1306::stopscroll(void) {
    ssd1306_command(SSD1306_DEACTIVATE_SCROLL);
}

bool Adafruit_SSD1306::getPixel(int16_t x, int16_t y) {
    if ((x < 0) || (x >= width()) || (y < 0) || (y >= height())) return false;
    return (buffer[x + (y / 8) * WIDTH] & (1 << (y & 7))) != 0;
}

uint8_t *Adafruit_SSD1306::getBuffer(void) {
    return buffer;
}

void Adafruit_SSD1306::SPIwrite(uint8_t d) {
}