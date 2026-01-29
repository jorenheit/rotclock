#pragma once
#include "util.h"
#include "time.h"

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
    return (_state() != oldState) && (_handler() != nullptr);
  }
  static void handle() { _handler()(_state()); }

public:
  static void begin(HandlerType handler) {
    pinMode(SW_A, INPUT_PULLUP);
    pinMode(SW_B, INPUT_PULLUP);
    _handler() = handler;
    update();
    handle();
  }

  static void loop() {
    exec_throttled(DEBOUNCE_DELAY, update, handle);
  }

  static State state() { return _state(); }
};

namespace StepperMotorHelper_ {
  enum Coil: uint8_t { A = 0b1100, B = 0b0011 };
  constexpr uint8_t operator+(Coil C) { return C & 0b1010; }
  constexpr uint8_t operator-(Coil C) { return C & 0b0101; }
  constexpr uint8_t operator~(Coil C) { return 0;          }

 union CoilConfig {
    uint8_t _val;
    struct { uint8_t A_1: 1; uint8_t A_2: 1; uint8_t B_1: 1; uint8_t B_2: 1; } get;
  };
}

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

  static void halfStep(Direction dir = CounterClockwise) {
    static uint8_t index = 0;
    index = (index + (dir == Clockwise ? 1 : 7)) & 7;
    setCoils(index);
  }

private:
  static void setCoils(uint8_t index) {
    using namespace StepperMotorHelper_;
    static constexpr CoilConfig const sequence[8] = {
      +A | ~B,
      +A | +B,
      ~A | +B,
      -A | +B,
      -A | ~B,
      -A | -B,
      ~A | -B,
      +A | -B
    };

    CoilConfig const &value = sequence[index & 7];
    digitalWrite<CA1>(value.get.A_1);
    digitalWrite<CA2>(value.get.A_2);
    digitalWrite<CB1>(value.get.B_1);
    digitalWrite<CB2>(value.get.B_2);
  }
};

using Millis = uint32_t();
template <typename Motor, typename Switch, uint16_t ClockTeeth, uint16_t GearTeeth, Millis millis>
class ClockTurner {
  static_assert(ClockTeeth % GearTeeth == 0);
  static constexpr uint64_t HALFSTEPS_PER_CLOCK_REVOLUTION = 2 * (ClockTeeth / GearTeeth) * Motor::STEPS_PER_REVOLUTION;
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
      if (_period() == -1) panic(InvalidSwitchState);
    });
  }

  static void loop() {
    static exec::Handle handle = nullptr;
    static uint64_t accumulator = 0;

    Switch::loop();
    if (!_initialized()) {
      _initialized() = true;
      exec::reset(handle);
      accumulator = 0;
    }

    auto result = exec_every_with(millis, 10, [](uint32_t dt){
        accumulator += dt * HALFSTEPS_PER_CLOCK_REVOLUTION;
        if (accumulator >= _period()) {
          accumulator -= _period();
          Motor::halfStep();
        }        
      });
    if (!handle) handle = exec::getHandle(result);
  }

  static void wobble(uint8_t const n, uint16_t const delayMillis) {
    for (uint8_t i = 0; i != n; ++i) { Motor::halfStep(Clockwise);        delay(delayMillis); }
    for (uint8_t i = 0; i != n; ++i) { Motor::halfStep(CounterClockwise); delay(delayMillis); }
  }
};

