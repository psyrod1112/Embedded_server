#pragma once
//
//    FILE: HX711.h
//  AUTHOR: Rob Tillaart (Modified for Raspberry Pi)
// VERSION: 0.6.3
// PURPOSE: Library for load cells for Raspberry Pi (Pure C++)
//

#include <stdint.h>
#include <string.h>

#define HX711_LIB_VERSION               "0.6.3"

const uint8_t HX711_AVERAGE_MODE = 0x00;
//  in median mode only between 3 and 15 samples are allowed.
const uint8_t HX711_MEDIAN_MODE  = 0x01;
//  medavg = average of the middle "half" of sorted elements
//  in medavg mode only between 3 and 15 samples are allowed.
const uint8_t HX711_MEDAVG_MODE  = 0x02;
//  runavg = running average
const uint8_t HX711_RUNAVG_MODE  = 0x03;
//  causes read() to be called only once!
const uint8_t HX711_RAW_MODE     = 0x04;

//  supported values for set_gain()
const uint8_t HX711_CHANNEL_A_GAIN_128 = 128;  //  default
const uint8_t HX711_CHANNEL_A_GAIN_64 = 64;
const uint8_t HX711_CHANNEL_B_GAIN_32 = 32;

class HX711
{
public:
  HX711();
  ~HX711();

  //  fixed gain 128 for now
  void     begin(uint8_t dataPin, uint8_t clockPin,
                 bool fastProcessor = false,
                 bool doReset = true);

  void     reset();

  //  checks if load cell is ready to read.
  bool     is_ready();

  //  wait until ready, check every ms
  void     wait_ready(uint32_t ms = 0);
  //  max # retries
  bool     wait_ready_retry(uint8_t retries = 3, uint32_t ms = 0);
  //  max timeout
  bool     wait_ready_timeout(uint32_t timeout = 1000, uint32_t ms = 0);

  ///////////////////////////////////////////////////////////////
  //  READ
  float    read();
  float    read_average(uint8_t times = 10);
  float    read_median(uint8_t times = 7);
  float    read_medavg(uint8_t times = 7);
  float    read_runavg(uint8_t times = 7, float alpha = 0.5);

  ///////////////////////////////////////////////////////////////
  //  MODE
  void     set_raw_mode();
  void     set_average_mode();
  void     set_median_mode();
  void     set_medavg_mode();
  void     set_runavg_mode();
  uint8_t  get_mode();

  float    get_value(uint8_t times = 1);
  float    get_units(uint8_t times = 1);

  ///////////////////////////////////////////////////////////////
  //  GAIN
  bool     set_gain(uint8_t gain = HX711_CHANNEL_A_GAIN_128, bool forced = false);
  uint8_t  get_gain();

  ///////////////////////////////////////////////////////////////
  //  TARE
  void     tare(uint8_t times = 10);
  float    get_tare();
  bool     tare_set();

  ///////////////////////////////////////////////////////////////
  //  CALIBRATION
  bool     set_scale(float scale = 1.0);
  float    get_scale();

  void     set_offset(int32_t offset = 0);
  int32_t  get_offset();

  void     calibrate_scale(float weight, uint8_t times = 10);

  ///////////////////////////////////////////////////////////////
  //  POWER MANAGEMENT
  void     power_down();
  void     power_up();

  ///////////////////////////////////////////////////////////////
  //  EXPERIMENTAL
  void     set_rate_pin(uint8_t pin);
  void     set_rate_10SPS();
  void     set_rate_80SPS();
  uint8_t  get_rate();

  //  TIME OF LAST READ
  uint32_t last_time_read();
  
  [[deprecated("Use last_time_read() instead.")]]
  uint32_t last_read() { return last_time_read(); };

  //  PRICING
  float    get_price(uint8_t times = 1) { return get_units(times) * _price; };
  void     set_unit_price(float price = 1.0) { _price = price; };
  float    get_unit_price() { return _price; };

private:
  uint8_t  _dataPin;
  uint8_t  _clockPin;

  int32_t  _offset;
  float    _scale;
  uint8_t  _gain;
  uint32_t _lastTimeRead;
  uint8_t  _mode;
  bool     _fastProcessor;
  uint8_t  _ratePin = 255;
  uint8_t  _rate = 10;
  float    _price;

  void     _insertSort(float * array, uint8_t size);
  uint8_t  _shiftIn();

  // --- 라즈베리 파이 하드웨어 제어용 Pure C++ Sysfs 가상 파일 인터페이스 ---
  bool     gpioExport(int pin);
  bool     gpioUnexport(int pin);
  bool     gpioSetDirection(int pin, const std::string& direction);
  bool     gpioSetValue(int pin, int value);
  int      gpioGetValue(int pin);
};

//  -- END OF FILE --