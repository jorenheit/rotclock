#pragma once
#include "fastdigitalread.h"
#include "fastdigitalwrite.h"

void panic();

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

inline char const *get_input(char *buf, int n) {
  buf[0] = 0;
  if (!Serial.available()) return buf;

  size_t len = Serial.readBytesUntil('\n', buf, n - 1);
  buf[len] = 0;
  if (len > 0 && buf[len - 1] == '\r') buf[len - 1] = 0;
  return buf;
}

template <int BufSize>
inline char const *get_input(char (&buf)[BufSize]) {
  return get_input(buf, BufSize);
}


