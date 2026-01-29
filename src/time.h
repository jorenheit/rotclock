#pragma once
#include <Wire.h>
#include "util.h"

namespace time {
struct TimeVal {
  uint8_t h, m, s;

  inline TimeVal(uint32_t sec = 0) {
    sec %= (24UL * 3600UL);
    h = sec / 3600u; sec %= 3600u;
    m = sec / 60u;   sec %= 60;
    s = sec % 60u;
  }

  inline TimeVal(uint8_t h_, uint8_t m_, uint8_t s_):
    m(m_), h(h_), s(s_)
  {
    (*this) = TimeVal{secondsSinceMidnight()};
  }

  inline TimeVal(TimeVal const &other) = default;
  TimeVal &operator=(TimeVal const &other) = default;

  template <uint8_t BufSize>
  inline char const *to_string(char (&buf)[BufSize]) const {
    static_assert(BufSize >= 9, "Buffer must be at least 9 bytes in size.");
    return to_string_raw(buf);
  }

  inline char const *to_string_tmp() const {
    static char buf[9];
    return to_string_raw(buf);
  }

  inline char const *to_string_raw(char *buf) const {
    auto const insert = [&buf](uint8_t idx, uint8_t const field) {
      buf[idx    ] = (field / 10) + '0';
      buf[idx + 1] = (field % 10) + '0';
    };

    insert(0, h); buf[2] = ':';
    insert(3, m); buf[5] = ':';
    insert(6, s); buf[8] = '\0';
    return buf;
  }

  inline uint32_t secondsSinceMidnight() const {
    return static_cast<uint32_t>(h) * 3600UL +
           static_cast<uint32_t>(m) * 60UL   +
           static_cast<uint32_t>(s);
  }

  inline operator uint32_t() const {
    return secondsSinceMidnight();
  }
};


template <uint8_t I2C_ADDR>
class RTC_DS3231 {
  	RTC_DS3231() = delete;

public:
  static void begin() {
    Wire.begin();
  }

  static bool good() { return _good(); }

  static TimeVal time() {
    // Set register pointer to seconds (0x00)
    Wire.beginTransmission(I2C_ADDR);
    Wire.write((uint8_t)0x00);
    if (Wire.endTransmission() != 0) panic(I2CError);

    // Read seconds, minutes, hours (3 bytes)
    if (Wire.requestFrom(I2C_ADDR, (uint8_t)3) != 3) panic(I2CError);
    uint8_t const s = bcd2dec(Wire.read() & 0x7F);   // mask CH bit
    uint8_t const m = bcd2dec(Wire.read() & 0x7F);
    uint8_t const h = bcd2dec(Wire.read() & 0x3F);  // 24h mode

    if (s >= 60 || m >= 60 || h >= 24) panic(I2CError);
    return TimeVal{h, m, s};
  }

  template <typename ... Args>
  static void set(Args ... args) {
    TimeVal t{args...};
    Wire.beginTransmission(I2C_ADDR);
    Wire.write((uint8_t)0x00);              // start at seconds
    Wire.write(dec2bcd(t.s));               // CH = 0
    Wire.write(dec2bcd(t.m));
    Wire.write(dec2bcd(t.h) & (uint8_t)0x3F); // 24h mode
    if (Wire.endTransmission() != 0) panic(I2CError);
  }

private:
  static uint8_t bcd2dec(uint8_t v) {
    return uint8_t((v >> 4) * 10 + (v & 0x0F));
  }

  static uint8_t dec2bcd(uint8_t v) {
    return uint8_t(((v / 10) << 4) | (v % 10));
  }
};

template <uint32_t NudgeOnSyncMillis, uint8_t MaxAllowedDesyncSeconds>
class ITC {
  static TimeVal &_startTime()    { static TimeVal startTime = 0;    return startTime;   }
  static uint32_t &_startMillis() { static uint32_t startMillis = 0; return startMillis; }
  ITC() = delete;

public:
  static void begin() {}

  template <typename ... Args>
  static void set(Args... args) {
    _startMillis() = ::millis();
    _startTime() = TimeVal{args...};
  }
  
  static uint32_t millis() {
    static uint32_t prevTime = 0;
    uint32_t currentTime = ::millis() - _startMillis();
    return (currentTime - prevTime < 0x80000000UL) ? (prevTime = currentTime) : prevTime;
  }

  static TimeVal time() {
    uint32_t const elapsedSeconds = ITC::millis() / 1000;
    uint32_t const startSeconds = _startTime().h * 3600UL + _startTime().m * 60UL + _startTime().s;
    return startSeconds + elapsedSeconds;
  }

  template <typename RTC>
  static void sync() {

    static auto const getClockDifference = [](){
      constexpr int32_t const SECONDS_PER_DAY = 24 * 3600;
      auto const itc = static_cast<int32_t>(ITC::time());
      auto const rtc = static_cast<int32_t>(RTC::time());
      int32_t dt = itc - rtc;
      if (dt >  SECONDS_PER_DAY / 2) dt -= SECONDS_PER_DAY;
      if (dt < -SECONDS_PER_DAY / 2) dt += SECONDS_PER_DAY;
      return dt;
    };

    // Compute time difference and adjust the start-value to compensate
    while (true) {
      int32_t const dt = getClockDifference();
      if (dt == 0) break;
      else if (abs(dt) > MaxAllowedDesyncSeconds) panic(MaxDesyncExceeded);
      int8_t const sgn = (dt > 0) ? 1 : (dt < 0) ? -1 : 0;
      _startMillis() += sgn * NudgeOnSyncMillis;
    }
  }
};

} // namespace time
