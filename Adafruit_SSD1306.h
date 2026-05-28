/*!
 * @file Adafruit_SSD1306.h
 *
 * This is part of for Adafruit's SSD1306 library for monochrome
 * OLED displays: http://www.adafruit.com/category/63_98
 *
 * These displays use I2C or SPI to communicate. I2C requires 2 pins
 * (SCL+SDA) and optionally a RESET pin. SPI requires 4 pins (MOSI, SCK,
 * select, data/command) and optionally a reset pin. Hardware SPI or
 * 'bitbang' software SPI are both supported.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries, with
 * contributions from the open source community.
 *
 * BSD license, all text above, and the splash screen header file,
 * must be included in any redistribution.
 *
 */

#ifndef _Adafruit_SSD1306_H_
#define _Adafruit_SSD1306_H_

// 아두이노 내장 헤더 수동 무력화 보정
#define ARDUINO 0

// ONE of the following three lines must be #defined:
#define SSD1306_128_64 ///< DEPRECTAED: old way to specify 128x64 screen
//#define SSD1306_128_32 ///< DEPRECATED: old way to specify 128x32 screen
// #define SSD1306_96_16   ///< DEPRECATED: old way to specify 96x16 screen

// Uncomment to disable Adafruit splash logo
// #define SSD1306_NO_SPLASH

#include "Adafruit_GFX.h"  // 👈 꺾쇠를 큰따옴표로 변경!
//#include <SPI.h>          // 👈 아두이노 전용 SPI 헤더 주석 처리
//#include <Wire.h> 

#if defined(__AVR__)
typedef volatile uint8_t PortReg;
typedef uint8_t PortMask;
#define HAVE_PORTREG
#elif defined(__SAM3X8E__)
typedef volatile RwReg PortReg;
typedef uint32_t PortMask;
#define HAVE_PORTREG
#elif defined(ARDUINO_ARCH_RTTHREAD)
#undef HAVE_PORTREG
#elif (defined(__arm__) || defined(ARDUINO_FEATHER52)) &&                      \
    !defined(ARDUINO_ARCH_MBED) && !defined(ARDUINO_ARCH_RP2040)
typedef volatile uint32_t PortReg;
typedef uint32_t PortMask;
#define HAVE_PORTREG
#endif

#ifndef NO_ADAFRUIT_SSD1306_COLOR_COMPATIBILITY
#define BLACK SSD1306_BLACK     ///< Draw 'off' pixels
#define WHITE SSD1306_WHITE     ///< Draw 'on' pixels
#define INVERSE SSD1306_INVERSE ///< Invert pixels
#endif

#define SSD1306_BLACK 0   ///< Draw 'off' pixels
#define SSD1306_WHITE 1   ///< Draw 'on' pixels
#define SSD1306_INVERSE 2 ///< Invert pixels

#define SSD1306_MEMORYMODE 0x20          ///< See datasheet
#define SSD1306_COLUMNADDR 0x21          ///< See datasheet
#define SSD1306_PAGEADDR 0x22            ///< See datasheet
#define SSD1306_SETCONTRAST 0x81         ///< See datasheet
#define SSD1306_CHARGEPUMP 0x8D          ///< See datasheet
#define SSD1306_SEGREMAP 0xA0            ///< See datasheet
#define SSD1306_DISPLAYALLON_RESUME 0xA4 ///< See datasheet
#define SSD1306_DISPLAYALLON 0xA5        ///< Not currently used
#define SSD1306_NORMALDISPLAY 0xA6       ///< See datasheet
#define SSD1306_INVERTDISPLAY 0xA7       ///< See datasheet
#define SSD1306_SETMULTIPLEX 0xA8        ///< See datasheet
#define SSD1306_DISPLAYOFF 0xAE          ///< See datasheet
#define SSD1306_DISPLAYON 0xAF           ///< See datasheet
#define SSD1306_COMSCANINC 0xC0          ///< Not currently used
#define SSD1306_COMSCANDEC 0xC8          ///< See datasheet
#define SSD1306_SETDISPLAYOFFSET 0xD3    ///< See datasheet
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5  ///< See datasheet
#define SSD1306_SETPRECHARGE 0xD9        ///< See datasheet
#define SSD1306_SETCOMPINS 0xDA          ///< See datasheet
#define SSD1306_SETVCOMDETECT 0xDB       ///< See datasheet

#define SSD1306_SETLOWCOLUMN 0x00  ///< Not currently used
#define SSD1306_SETHIGHCOLUMN 0x10 ///< Not currently used
#define SSD1306_SETSTARTLINE 0x40  ///< See datasheet

#define SSD1306_EXTERNALVCC 0x01  ///< External display voltage source
#define SSD1306_SWITCHCAPVCC 0x02 ///< Gen. display voltage from 3.3V

#define SSD1306_RIGHT_HORIZONTAL_SCROLL 0x26              ///< Init rt scroll
#define SSD1306_LEFT_HORIZONTAL_SCROLL 0x27               ///< Init left scroll
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29 ///< Init diag scroll
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A  ///< Init diag scroll
#define SSD1306_DEACTIVATE_SCROLL 0x2E                    ///< Stop scroll
#define SSD1306_ACTIVATE_SCROLL 0x2F                      ///< Start scroll
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3             ///< Set scroll range

#if defined SSD1306_128_64
#define SSD1306_LCDWIDTH 128 
#define SSD1306_LCDHEIGHT 64 
#endif
#if defined SSD1306_128_32
#define SSD1306_LCDWIDTH 128 
#define SSD1306_LCDHEIGHT 32 
#endif
#if defined SSD1306_96_16
#define SSD1306_LCDWIDTH 96  
#define SSD1306_LCDHEIGHT 16 
#endif

/*!
    @brief  Class that stores state and functions for interacting with
            SSD1306 OLED displays.
*/
class Adafruit_SSD1306 : public Adafruit_GFX {
public:
  // 👈 TwoWire, SPIClass 제거 및 void* 포인터 뼈대로 보정
  Adafruit_SSD1306(uint8_t w, uint8_t h, void *twi = NULL,
                   int8_t rst_pin = -1, uint32_t clkDuring = 400000UL,
                   uint32_t clkAfter = 100000UL);
  Adafruit_SSD1306(uint8_t w, uint8_t h, int8_t mosi_pin, int8_t sclk_pin,
                   int8_t dc_pin, int8_t rst_pin, int8_t cs_pin);
  Adafruit_SSD1306(uint8_t w, uint8_t h, void *spi, int8_t dc_pin,
                   int8_t rst_pin, int8_t cs_pin, uint32_t bitrate = 8000000UL);

  Adafruit_SSD1306(int8_t mosi_pin, int8_t sclk_pin, int8_t dc_pin,
                   int8_t rst_pin, int8_t cs_pin);
  Adafruit_SSD1306(int8_t dc_pin, int8_t rst_pin, int8_t cs_pin);
  Adafruit_SSD1306(int8_t rst_pin = -1);

  ~Adafruit_SSD1306(void);

  bool begin(uint8_t switchvcc = SSD1306_SWITCHCAPVCC, uint8_t i2caddr = 0,
             bool reset = true, bool periphBegin = true);
  void display(void);
  void clearDisplay(void);
  void invertDisplay(bool i);
  void dim(bool dim);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void startscrollright(uint8_t start, uint8_t stop);
  void startscrollleft(uint8_t start, uint8_t stop);
  void startscrolldiagright(uint8_t start, uint8_t stop);
  void startscrolldiagleft(uint8_t start, uint8_t stop);
  void stopscroll(void);
  void ssd1306_command(uint8_t c);
  bool getPixel(int16_t x, int16_t y);
  uint8_t *getBuffer(void);

protected:
  inline void SPIwrite(uint8_t d) __attribute__((always_inline));
  void drawFastHLineInternal(int16_t x, int16_t y, int16_t w, uint16_t color);
  void drawFastVLineInternal(int16_t x, int16_t y, int16_t h, uint16_t color);
  void ssd1306_command1(uint8_t c);
  void ssd1306_commandList(const uint8_t *c, uint8_t n);

  // 👈 내부 멤버 통신 변수들도 void* 로 타입 변경
  void *spi;   
  void *wire;   
  uint8_t *buffer; 
  int8_t i2caddr;  
  int8_t vccstate; 
  int8_t page_end; 
  int8_t mosiPin;  
  int8_t clkPin;   
  int8_t dcPin;    
  int8_t csPin; 
  int8_t rstPin; 

#ifdef HAVE_PORTREG
  PortReg *mosiPort, *clkPort, *dcPort, *csPort;
  PortMask mosiPinMask, clkPinMask, dcPinMask, csPinMask;
#endif
#if ARDUINO >= 157
  uint32_t wireClk;    
  uint32_t restoreClk; 
#endif
  uint8_t contrast; 
};

#endif // _Adafruit_SSD1306_H_