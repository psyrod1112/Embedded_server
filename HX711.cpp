//
//    FILE: HX711.cpp
//  AUTHOR: Rob Tillaart (Modified for Raspberry Pi)
// VERSION: 0.6.3
// PURPOSE: Library for load cells for Raspberry Pi (Pure C++)
//

#include "HX711.h"
#include <fstream>
#include <chrono>
#include <thread>
#include <iostream>

// 리눅스 시스템 시간(ms)을 가져오는 헬퍼 함수 (아두이노의 millis() 대체)
static uint32_t current_millis() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

// 마이크로초 지연 함수
inline void delayMicroseconds(int us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

// 밀리초 지연 함수 (아두이노의 delay() 대체)
inline void delay(int ms) {
    if (ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}

// CPU 스레드 양보 함수 (아두이노의 yield() 대체)
inline void yield() {
    std::this_thread::yield();
}

HX711::HX711()
{
  _offset   = 0;
  _scale    = 1;
  _gain     = HX711_CHANNEL_A_GAIN_128;
  _lastTimeRead = 0;
  _mode     = HX711_AVERAGE_MODE;
  _fastProcessor = false;
  _price    = 0;
}

HX711::~HX711()
{
  gpioUnexport(_dataPin);
  gpioUnexport(_clockPin);
  if (_ratePin != 255) {
      gpioUnexport(_ratePin);
  }
}

void HX711::begin(uint8_t dataPin, uint8_t clockPin, bool fastProcessor, bool doReset)
{
  _dataPin  = dataPin;
  _clockPin = clockPin;
  _fastProcessor = fastProcessor;

  // 라즈베리 파이 GPIO 파일 시스템 오픈 (Export)
  gpioExport(_dataPin);
  gpioExport(_clockPin);
  
  // 리눅스가 파일 권한을 설정할 수 있도록 아주 아주 잠깐 대기
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // 방향 설정 (Data = Input, Clock = Output)
  gpioSetDirection(_dataPin, "in");
  gpioSetDirection(_clockPin, "out");
  
  // 클럭 초기 상태 Low로 설정
  gpioSetValue(_clockPin, 0);

  if (doReset)
  {
    reset();
  }
}

void HX711::reset()
{
  power_down();
  power_up();
  _offset   = 0;
  _scale    = 1;
  _gain     = HX711_CHANNEL_A_GAIN_128;
  _lastTimeRead = 0;
  _mode     = HX711_AVERAGE_MODE;
  _price    = 0;
}

bool HX711::is_ready()
{
  return gpioGetValue(_dataPin) == 0; // LOW 상태일 때 데이터 준비 완료
}

void HX711::wait_ready(uint32_t ms)
{
  while (!is_ready())
  {
    delay(ms);
  }
}

bool HX711::wait_ready_retry(uint8_t retries, uint32_t ms)
{
  while (retries--)
  {
    if (is_ready()) return true;
    delay(ms);
  }
  return false;
}

bool HX711::wait_ready_timeout(uint32_t timeout, uint32_t ms)
{
  uint32_t start = current_millis();
  while (current_millis() - start < timeout)
  {
    if (is_ready()) return true;
    delay(ms);
  }
  return false;
}

float HX711::read()
{
  // 데이터 준비될 때까지 블로킹 대기
  while (gpioGetValue(_dataPin) == 1)
  {
    yield();
  }

  union
  {
    int32_t value = 0;
    uint8_t data[4];
  } v;

  // 24비트 데이터를 8비트씩 3번에 걸쳐 비트 뱅잉으로 읽어옴
  v.data[2] = _shiftIn();
  v.data[1] = _shiftIn();
  v.data[0] = _shiftIn();

  uint8_t m = 1;
  if      (_gain == HX711_CHANNEL_A_GAIN_128) m = 1;
  else if (_gain == HX711_CHANNEL_A_GAIN_64)  m = 3;
  else if (_gain == HX711_CHANNEL_B_GAIN_32)  m = 2;

  while (m > 0)
  {
    gpioSetValue(_clockPin, 1);
    if (_fastProcessor) delayMicroseconds(1);
    gpioSetValue(_clockPin, 0);
    if (_fastProcessor) delayMicroseconds(1);
    m--;
  }

  // 24비트 정수 음수 표현 부호 확장 (SIGN extend)
  if (v.data[2] & 0x80) v.data[3] = 0xFF;

  _lastTimeRead = current_millis();
  return 1.0 * v.value;
}

float HX711::read_average(uint8_t times)
{
  if (times < 1) times = 1;
  float sum = 0;
  for (uint8_t i = 0; i < times; i++)
  {
    sum += read();
    yield();
  }
  return sum / times;
}

float HX711::read_median(uint8_t times)
{
  if (times > 15) times = 15;
  if (times < 3)  times = 3;
  float samples[15];
  for (uint8_t i = 0; i < times; i++)
  {
    samples[i] = read();
    yield();
  }
  _insertSort(samples, times);
  if (times & 0x01) return samples[times/2];
  return (samples[times/2] + samples[times/2 + 1]) / 2;
}

float HX711::read_medavg(uint8_t times)
{
  if (times > 15) times = 15;
  if (times < 3)  times = 3;
  float samples[15];
  for (uint8_t i = 0; i < times; i++)
  {
    samples[i] = read();
    yield();
  }
  _insertSort(samples, times);
  float sum = 0;
  uint8_t count = 0;
  uint8_t first = (times + 2) / 4;
  uint8_t last  = times - first - 1;
  for (uint8_t i = first; i <= last; i++)
  {
    sum += samples[i];
    count++;
  }
  return sum / count;
}

float HX711::read_runavg(uint8_t times, float alpha)
{
  if (times < 1)  times = 1;
  if (alpha < 0)  alpha = 0;
  if (alpha > 1)  alpha = 1;
  float val = read();
  for (uint8_t i = 1; i < times; i++)
  {
    val += alpha * (read() - val);
    yield();
  }
  return val;
}

void HX711::set_raw_mode() { _mode = HX711_RAW_MODE; }
void HX711::set_average_mode() { _mode = HX711_AVERAGE_MODE; }
void HX711::set_median_mode() { _mode = HX711_MEDIAN_MODE; }
void HX711::set_medavg_mode() { _mode = HX711_MEDAVG_MODE; }
void HX711::set_runavg_mode() { _mode = HX711_RUNAVG_MODE; }
uint8_t HX711::get_mode() { return _mode; }

float HX711::get_value(uint8_t times)
{
  float raw;
  switch(_mode)
  {
    case HX711_RAW_MODE:        raw = read(); break;
    case HX711_RUNAVG_MODE:     raw = read_runavg(times); break;
    case HX711_MEDAVG_MODE:     raw = read_medavg(times); break;
    case HX711_MEDIAN_MODE:     raw = read_median(times); break;
    case HX711_AVERAGE_MODE:
    default:                    raw = read_average(times); break;
  }
  return raw - _offset;
}

float HX711::get_units(uint8_t times)
{
  float units = get_value(times) * _scale;
  return units;
}

bool HX711::set_gain(uint8_t gain, bool forced)
{
  if ((!forced) && (_gain == gain)) return true;
  switch(gain)
  {
    case HX711_CHANNEL_B_GAIN_32:
    case HX711_CHANNEL_A_GAIN_64:
    case HX711_CHANNEL_A_GAIN_128:
      _gain = gain;
      read();
      return true;
  }
  return false;
}

uint8_t HX711::get_gain() { return _gain; }

void HX711::tare(uint8_t times)
{
  _offset = read_average(times);
}

float HX711::get_tare() { return -_offset * _scale; }
bool HX711::tare_set() { return _offset != 0; }

bool HX711::set_scale(float scale)
{
  if (scale == 0) return false;
  _scale = 1.0 / scale;
  return true;
}

float HX711::get_scale() { return 1.0 / _scale; }
void HX711::set_offset(int32_t offset) { _offset = offset; }
int32_t HX711::get_offset() { return _offset; }

void HX711::calibrate_scale(float weight, uint8_t times)
{
  _scale = weight / (read_average(times) - _offset);
}

void HX711::power_down()
{
  gpioSetValue(_clockPin, 1);
  delayMicroseconds(64);
}

void HX711::power_up()
{
  gpioSetValue(_clockPin, 0);
}

void HX711::set_rate_pin(uint8_t pin)
{
  _ratePin = pin;
  gpioExport(_ratePin);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  gpioSetDirection(_ratePin, "out");
  set_rate_10SPS();
}

void HX711::set_rate_10SPS()
{
  _rate = 10;
  if (_ratePin != 255) gpioSetValue(_ratePin, 0);
}

void HX711::set_rate_80SPS()
{
  _rate = 80;
  if (_ratePin != 255) gpioSetValue(_ratePin, 1);
}

uint8_t HX711::get_rate() { return _rate; }

uint32_t HX711::last_time_read() { return _lastTimeRead; }

void HX711::_insertSort(float * array, uint8_t size)
{
  uint8_t t, z;
  float temp;
  for (t = 1; t < size; t++)
  {
    z = t;
    temp = array[z];
    while ((z > 0) && (temp < array[z - 1]))
    {
      array[z] = array[z - 1];
      z--;
    }
    array[z] = temp;
    yield();
  }
}

uint8_t HX711::_shiftIn()
{
  uint8_t clk   = _clockPin;
  uint8_t data  = _dataPin;
  uint8_t value = 0;
  uint8_t mask  = 0x80;
  while (mask > 0)
  {
    gpioSetValue(clk, 1);
    if (_fastProcessor) delayMicroseconds(1);
    if (gpioGetValue(data) == 1)
    {
      value |= mask;
    }
    gpioSetValue(clk, 0);
    if (_fastProcessor) delayMicroseconds(1);
    mask >>= 1;
  }
  return value;
}

// --- 하드웨어 제어용 가상 파일 제어 실체 구현 (Pure C++ Sysfs) ---

bool HX711::gpioExport(int pin) {
    std::ofstream f("/sys/class/gpio/export");
    if (!f) return false;
    f << pin;
    return true;
}

bool HX711::gpioUnexport(int pin) {
    std::ofstream f("/sys/class/gpio/unexport");
    if (!f) return false;
    f << pin;
    return true;
}

bool HX711::gpioSetDirection(int pin, const std::string& direction) {
    std::ofstream f("/sys/class/gpio/gpio" + std::to_string(pin) + "/direction");
    if (!f) return false;
    f << direction;
    return true;
}

bool HX711::gpioSetValue(int pin, int value) {
    std::ofstream f("/sys/class/gpio/gpio" + std::to_string(pin) + "/value");
    if (!f) return false;
    f << value;
    return true;
}

int HX711::gpioGetValue(int pin) {
    std::ifstream f("/sys/class/gpio/gpio" + std::to_string(pin) + "/value");
    if (!f) return -1;
    int val;
    f >> val;
    return val;
}
//  -- END OF FILE --