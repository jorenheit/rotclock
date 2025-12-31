#pragma once
#include "fastdigitalread.h"
#include "fastdigitalwrite.h"

template <uint8_t CA1, uint8_t CA2, uint8_t CB1, uint8_t CB2>
class StepperMotor {
public:
  enum Direction {
    Clockwise,
    CounterClockwise
  };

  void begin() {
    pinMode(CA1, OUTPUT);
    pinMode(CA2, OUTPUT);
    pinMode(CB1, OUTPUT);
    pinMode(CB2, OUTPUT);

    setCoils(0);
  }

  void setCoils(uint8_t index) {
      // Sequence for half-stepping clockwise
    static constexpr uint8_t const sequence[8][4] = {
      {1, 0, 0, 0}, // A+  B0
      {1, 0, 1, 0}, // A+  B+
      {0, 0, 1, 0}, // A0  B+
      {0, 1, 1, 0}, // A-  B+
      {0, 1, 0, 0}, // A-  B0
      {0, 1, 0, 1}, // A-  B-
      {0, 0, 0, 1}, // A0  B-
      {1, 0, 0, 1}  // A+  B-
    }; 

    digitalWrite<CA1>(sequence[index][0]);
    digitalWrite<CA2>(sequence[index][1]);
    digitalWrite<CB1>(sequence[index][2]);
    digitalWrite<CB2>(sequence[index][3]);
  }

  void halfStep(Direction dir) {
    static uint8_t index = 0;

    // Determine next motor position (index to the _sequence table)
    index = (index + (dir == Clockwise ? 1 : 7)) & 7;
    // Move motor into that position
    setCoils(index);
  }
};

template <typename Motor>
class ClockTurner {
  
  enum Params: uint32_t {
    CLOCK_TEETH_COUNT = 216,
    GEAR_TEETH_COUNT = 36,
    GEAR_RATIO = CLOCK_TEETH_COUNT / GEAR_TEETH_COUNT,
    STEPS_PER_MOTOR_REVOLUTION = 96,
    HALFSTEPS_PER_MOTOR_REVOLUTION = 2 * STEPS_PER_MOTOR_REVOLUTION,
    HALFSTEPS_PER_CLOCK_REVOLUTION = GEAR_RATIO * HALFSTEPS_PER_MOTOR_REVOLUTION
  };
  static_assert(CLOCK_TEETH_COUNT % GEAR_TEETH_COUNT == 0);

public:
  enum Hand {
    SecondHand,
    MinuteHand,
    HourHand
  };

private:
  Motor     _motor;
  Hand      _hand = HourHand;
  bool      _initialized = false;
  
public:
  ClockTurner() = default;
  ClockTurner(ClockTurner const &) = delete;
  ClockTurner(ClockTurner &&) = delete;
  ClockTurner &operator=(ClockTurner const &) = delete;
  ClockTurner &operator=(ClockTurner &&) = delete;

  void begin() {
    _motor.begin();
  }

  void loop() {
    static uint32_t accumulator = 0;
    static uint32_t lastTime = 0;
    static uint32_t period = 0;

    if (!_initialized) {
      _initialized = true;
      lastTime = millis();
      accumulator = 0;
      period = [&]() {
        switch (_hand) {
          case SecondHand: return 60UL * 1000UL;
          case MinuteHand: return 60UL * 60UL * 1000UL;
          case HourHand:   
          default:         return 12UL * 60UL * 60UL * 1000UL;
        }
      }();
      return;
    }

    uint32_t now = millis();
    uint32_t elapsed = now - lastTime;

    accumulator += elapsed * HALFSTEPS_PER_CLOCK_REVOLUTION;
    while (accumulator >= period) {
      accumulator -= period;
      _motor.halfStep(Motor::Clockwise);
    }
    lastTime = now;
  }

  void setStationaryHand(Hand hand) {
    _hand = hand;
    _initialized = false;
  }
};

template <uint8_t SW_A, uint8_t SW_B>
class Switch {
  static constexpr uint16_t DEBOUNCE_DELAY = 200;

public:
  enum State {
    Up        = 0b01, 
    Middle    = 0b11, 
    Down      = 0b10, 
    Undefined = 0b00
  };
  using HandlerType = void (*)(State);

private:
  HandlerType _handler = nullptr;
  State _state = Undefined;

public:
  void begin(HandlerType handler) {
    pinMode(SW_A, INPUT_PULLUP);
    pinMode(SW_B, INPUT_PULLUP);
    _handler = handler;
  }

  void loop() {
    static unsigned long prevTime = millis();
    unsigned long now = millis();
    if (now - prevTime < DEBOUNCE_DELAY) return;

    State newState = (digitalRead<SW_B>() << 1) | digitalRead<SW_A>();
    if (newState != _state) {
      _state = newState;
      prevTime = now;
      if (_handler != nullptr) _handler(newState);
    }
  }

  State getState() const {
    return _state;
  }
};
