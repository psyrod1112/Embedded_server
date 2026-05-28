#ifndef _ADAFRUIT_GFX_H
#define _ADAFRUIT_GFX_H

#include <stddef.h>  // 👈 .h 스타일로 변경
#include <string.h>  // 👈 .h 스타일로 변경
#include <stdlib.h>  // 👈 .h 스타일로 변경
#include <avr/io.h> // 👈 순수 AVR 입출력 헤더 기본 포함

// 아두이노 내장 헤더 의존성 제거
#define ARDUINO 0 

#include "gfxfont.h"

//#include <Adafruit_I2CDevice.h> 
//#include <Adafruit_SPIDevice.h> 

/// A generic graphics superclass that can handle all sorts of drawing.
class Adafruit_GFX { // 👈 아두이노의 ': public Print' 제거!

public:
  Adafruit_GFX(int16_t w, int16_t h); // Constructor

  virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;

  // TRANSACTION API / CORE DRAW API
  virtual void startWrite(void);
  virtual void writePixel(int16_t x, int16_t y, uint16_t color);
  virtual void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                             uint16_t color);
  virtual void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  virtual void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  virtual void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                         uint16_t color);
  virtual void endWrite(void);

  // CONTROL API
  virtual void setRotation(uint8_t r);
  virtual void invertDisplay(bool i);

  // BASIC DRAW API
  virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                        uint16_t color);
  virtual void fillScreen(uint16_t color);
  virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                        uint16_t color);
  virtual void drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
                        uint16_t color);

  // These exist only with Adafruit_GFX
  void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
  void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername,
                        uint16_t color);
  void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
  void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername,
                        int16_t delta, uint16_t color);
  void drawEllipse(int16_t x0, int16_t y0, int16_t rw, int16_t rh,
                   uint16_t color);
  void fillEllipse(int16_t x0, int16_t y0, int16_t rw, int16_t rh,
                   uint16_t color);
  void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2,
                    int16_t y2, uint16_t color);
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2,
                    int16_t y2, uint16_t color);
  void drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
                     int16_t radius, uint16_t color);
  void fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
                     int16_t radius, uint16_t color);
  void drawRotatedRect(int16_t cenX, int16_t cenY, int16_t w, int16_t h,
                       int16_t angleDeg, uint16_t color);
  void fillRotatedRect(int16_t cenX, int16_t cenY, int16_t w, int16_t h,
                       int16_t angleDeg, uint16_t color);
  void rotatePoint(int16_t &x0, int16_t &y0, int16_t angleDeg);
  void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w,
                  int16_t h, uint16_t color);
  void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w,
                  int16_t h, uint16_t color, uint16_t bg);
  void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h,
                  uint16_t color);
  void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h,
                  uint16_t color, uint16_t bg);
  void drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w,
                   int16_t h, uint16_t color);
  void drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                           int16_t w, int16_t h);
  void drawGrayscaleBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w,
                           int16_t h);
  void drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                           const uint8_t mask[], int16_t w, int16_t h);
  void drawGrayscaleBitmap(int16_t x, int16_t y, uint8_t *bitmap, uint8_t *mask,
                           int16_t w, int16_t h);
  void drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[], int16_t w,
                     int16_t h);
  void drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, int16_t w,
                     int16_t h);
  void drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[],
                     const uint8_t mask[], int16_t w, int16_t h);
  void drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, uint8_t *mask,
                     int16_t w, int16_t h);
  void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color,
                uint16_t bg, uint8_t size);
  void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color,
                uint16_t bg, uint8_t size_x, uint8_t size_y);
  
  void getTextBounds(const char *string, int16_t x, int16_t y, int16_t *x1,
                     int16_t *y1, uint16_t *w, uint16_t *h);

  // 👈 아두이노 전용 자료형 매개변수 함수들 통째로 주석 처리
  // void getTextBounds(const __FlashStringHelper *s, int16_t x, int16_t y,
  //                    int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);
  // void getTextBounds(const String &str, int16_t x, int16_t y, int16_t *x1,
  //                    int16_t *y1, uint16_t *w, uint16_t *h);
  
  void setTextSize(uint8_t s);
  void setTextSize(uint8_t sx, uint8_t sy);
  void setFont(const GFXfont *f = NULL);

  void setCursor(int16_t x, int16_t y) {
    cursor_x = x;
    cursor_y = y;
  }

  void setTextColor(uint16_t c) { textcolor = textbgcolor = c; }
  void setTextColor(uint16_t c, uint16_t bg) {
    textcolor = c;
    textbgcolor = bg;
  }

  void setTextWrap(bool w) { wrap = w; }
  void cp437(bool x = true) { _cp437 = x; }

  // 👈 아두이노 Print::write 및 가상 write 매커니즘 주석 처리
  // using Print::write;
  // virtual void write(uint8_t);
  
  // 우리 프로젝트에서 쓸 순수 문자열 출력용 함수 인터페이스 정의 추가
  //virtual size_t write(uint8_t c) { return 0; } 
  virtual size_t write(uint8_t c);

  int16_t width(void) const { return _width; };
  int16_t height(void) const { return _height; }
  uint8_t getRotation(void) const { return rotation; }
  int16_t getCursorX(void) const { return cursor_x; }
  int16_t getCursorY(void) const { return cursor_y; };

protected:
  void charBounds(unsigned char c, int16_t *x, int16_t *y, int16_t *minx,
                  int16_t *miny, int16_t *maxx, int16_t *maxy);
  int16_t WIDTH;        
  int16_t HEIGHT;       
  int16_t _width;       
  int16_t _height;      
  int16_t cursor_x;     
  int16_t cursor_y;     
  uint16_t textcolor;   
  uint16_t textbgcolor; 
  uint8_t textsize_x;   
  uint8_t textsize_y;   
  uint8_t rotation;     
  bool wrap;            
  bool _cp437;          
  GFXfont *gfxFont;     
};

/// A simple drawn button UI element
class Adafruit_GFX_Button {
public:
  Adafruit_GFX_Button(void);
  void initButton(Adafruit_GFX *gfx, int16_t x, int16_t y, uint16_t w,
                  uint16_t h, uint16_t outline, uint16_t fill,
                  uint16_t textcolor, char *label, uint8_t textsize);
  void initButton(Adafruit_GFX *gfx, int16_t x, int16_t y, uint16_t w,
                  uint16_t h, uint16_t outline, uint16_t fill,
                  uint16_t textcolor, char *label, uint8_t textsize_x,
                  uint8_t textsize_y);
  void initButtonUL(Adafruit_GFX *gfx, int16_t x1, int16_t y1, uint16_t w,
                    uint16_t h, uint16_t outline, uint16_t fill,
                    uint16_t textcolor, char *label, uint8_t textsize);
  void initButtonUL(Adafruit_GFX *gfx, int16_t x1, int16_t y1, uint16_t w,
                    uint16_t h, uint16_t outline, uint16_t fill,
                    uint16_t textcolor, char *label, uint8_t textsize_x,
                    uint8_t textsize_y);
  void drawButton(bool inverted = false);
  bool contains(int16_t x, int16_t y);

  void press(bool p) {
    laststate = currstate;
    currstate = p;
  }

  bool justPressed();
  bool justReleased();
  bool isPressed(void) { return currstate; };

private:
  Adafruit_GFX *_gfx;
  int16_t _x1, _y1; 
  uint16_t _w, _h;
  uint8_t _textsize_x;
  uint8_t _textsize_y;
  uint16_t _outlinecolor, _fillcolor, _textcolor;
  char _label[10];
  bool currstate, laststate;
};

/// A GFX 1-bit canvas context for graphics
class GFXcanvas1 : public Adafruit_GFX {
public:
  GFXcanvas1(uint16_t w, uint16_t h, bool allocate_buffer = true);
  ~GFXcanvas1(void);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void fillScreen(uint16_t color);
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  bool getPixel(int16_t x, int16_t y) const;
  uint8_t *getBuffer(void) const { return buffer; }

protected:
  bool getRawPixel(int16_t x, int16_t y) const;
  void drawFastRawVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void drawFastRawHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  uint8_t *buffer;   
  bool buffer_owned; 

private:
#ifdef __AVR__
  // 👈 컴파일러 매크로 오류 방지를 위해 PROGMEM 선언부 일반으로 보정
  static const uint8_t GFXsetBit[], GFXclrBit[];
#endif
};

/// A GFX 8-bit canvas context for graphics
class GFXcanvas8 : public Adafruit_GFX {
public:
  GFXcanvas8(uint16_t w, uint16_t h, bool allocate_buffer = true);
  ~GFXcanvas8(void);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void fillScreen(uint16_t color);
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  uint8_t getPixel(int16_t x, int16_t y) const;
  uint8_t *getBuffer(void) const { return buffer; }

protected:
  uint8_t getRawPixel(int16_t x, int16_t y) const;
  void drawFastRawVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void drawFastRawHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  uint8_t *buffer;   
  bool buffer_owned; 
};

///  A GFX 16-bit canvas context for graphics
class GFXcanvas16 : public Adafruit_GFX {
public:
  GFXcanvas16(uint16_t w, uint16_t h, bool allocate_buffer = true);
  ~GFXcanvas16(void);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void fillScreen(uint16_t color);
  void byteSwap(void);
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  uint16_t getPixel(int16_t x, int16_t y) const;
  uint16_t *getBuffer(void) const { return buffer; }

protected:
  uint16_t getRawPixel(int16_t x, int16_t y) const;
  void drawFastRawVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void drawFastRawHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  uint16_t *buffer;  
  bool buffer_owned; 
};

#endif // _ADAFRUIT_GFX_H