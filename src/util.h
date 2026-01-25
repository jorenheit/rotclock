#pragma once
#include "fastdigitalread.h"
#include "fastdigitalwrite.h"

enum PanicCode: uint8_t {
  I2CError = 1,
  MaxDesyncExceeded,
  InvalidSwitchState
};

void panic(PanicCode code);
inline char const *panicCodeString(PanicCode code) {
  static constexpr char const *str[] = {
    "I2C Error",
    "Maximum desynchronization exceeded.",
    "Invalid switch state."
  };

  return str[code];
}

template <uint32_t Key, uint32_t Value, uint32_t ... Rest> 
struct Map {
  static_assert(sizeof ... (Rest) % 2 == 0, "Odd number of arguments.");
  static uint32_t at(uint32_t const key) {
    return (Key == key) ? Value : Map<Rest ...>::at(key);
  }
};

template <uint32_t Key, uint32_t Value>
struct Map<Key, Value> {
  static uint32_t at(uint32_t const key) {
    return (Key == key) ? Value : -1;
  }
};

