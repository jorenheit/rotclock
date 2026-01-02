#pragma once
#include "util.h"

enum Direction {
  Clockwise,
  CounterClockwise
};

enum: uint32_t {
  MILLIS_PER_MINUTE   = 60UL * 1000UL,
  MILLIS_PER_HOUR     = 60UL * MILLIS_PER_MINUTE,
  MILLIS_PER_12_HOURS = 12UL * MILLIS_PER_HOUR
};

template <uint8_t SW_A, uint8_t SW_B>
class Switch {
  static constexpr uint16_t DEBOUNCE_DELAY = 50;

public:
  enum State {
    Up        = 0b01,
    Middle    = 0b11,
    Down      = 0b10,
    Undefined = 0b00
  };
  using HandlerType = void (*)(State);

private:
  Switch() = delete;
  
  static HandlerType &_handler() { static HandlerType h = nullptr; return h; }
  static State &_state() { static State s = Undefined; return s; }
  
  static bool update() { 
    State oldState = _state();
    _state() = (digitalRead<SW_B>() << 1) | digitalRead<SW_A>(); 
    return _state() != oldState;
  }

public:
  static void begin(HandlerType handler) {
    pinMode(SW_A, INPUT_PULLUP);
    pinMode(SW_B, INPUT_PULLUP);
    update();
    handler(state());
    _handler() = handler;
  }

  static void loop() {
    static unsigned long prevTime = millis();
    unsigned long now = millis();
    if (now - prevTime < DEBOUNCE_DELAY) return;
    if (update()) {
      prevTime = now;
      if (_handler() != nullptr) _handler()(state());
    }
  }

  static State state() {
    return _state();
  }
};

template <uint8_t CA1, uint8_t CA2, uint8_t CB1, uint8_t CB2, uint16_t Steps>
class StepperMotor {
  StepperMotor() = delete;

public:
  static constexpr uint16_t STEPS_PER_REVOLUTION = Steps;

  static void begin() {
    pinMode(CA1, OUTPUT);
    pinMode(CA2, OUTPUT);
    pinMode(CB1, OUTPUT);
    pinMode(CB2, OUTPUT);
    setCoils(0);
  }

  static void halfStep(Direction dir = Clockwise) {
    static uint8_t index = 0;
    index = (index + (dir == Clockwise ? 1 : 7)) & 7;
    setCoils(index);
  }

private:
  static void setCoils(uint8_t index) {
    // Sequence for half-stepping clockwise
    static constexpr uint8_t const sequence[8] = {
      0b1000, // A+  B0
      0b1010, // A+  B+
      0b0010, // A0  B+
      0b0110, // A-  B+
      0b0100, // A-  B0
      0b0101, // A-  B-
      0b0001, // A0  B-
      0b1001  // A+  B-
    };

    uint8_t const value = sequence[index & 7];
    digitalWrite<CA1>(value & 0b1000);
    digitalWrite<CA2>(value & 0b0100);
    digitalWrite<CB1>(value & 0b0010);
    digitalWrite<CB2>(value & 0b0001);
  }
};

template <typename Motor, typename Switch, uint16_t ClockTeeth, uint16_t GearTeeth>
class ClockTurner {
  static_assert(ClockTeeth % GearTeeth == 0);
  static constexpr uint32_t HALFSTEPS_PER_CLOCK_REVOLUTION = 2 * (ClockTeeth / GearTeeth) * Motor::STEPS_PER_REVOLUTION;
  using SwitchState = typename Switch::State;

  ClockTurner() = delete;
  static uint32_t &_period()      { static uint32_t p = -1; return p; }
  static bool     &_initialized() { static bool i = false;  return i; }

public:
  static void begin() {
    Motor::begin();
    Switch::begin(+[](SwitchState state){      
      _initialized() = false;
      _period() = Map<
        Switch::Up,     MILLIS_PER_MINUTE, 
        Switch::Middle, MILLIS_PER_HOUR, 
        Switch::Down,   MILLIS_PER_12_HOURS
      >::at(state);
    });
  }

  static void loop() {
    static uint32_t accumulator = 0;
    static uint32_t lastTime = 0;

    Switch::loop();
    if (!_initialized()) {
      _initialized() = true;
      accumulator = 0;
      lastTime = millis();
    }

    uint32_t now = millis();
    uint32_t elapsed = now - lastTime;
    lastTime = now;

    accumulator += elapsed * HALFSTEPS_PER_CLOCK_REVOLUTION;
    while (accumulator >= _period()) {
      accumulator -= _period();
      Motor::halfStep();
    }
  }
};
