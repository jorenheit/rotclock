#pragma once
#include "fastdigitalread.h"
#include "fastdigitalwrite.h"

enum Hand {
  SecondHand,
  MinuteHand,
  HourHand
};

enum Direction {
  Clockwise,
  CounterClockwise
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
  static State &_state()         { static State s = Undefined;     return s; }

public:
  static void begin(HandlerType handler) {
    pinMode(SW_A, INPUT_PULLUP);
    pinMode(SW_B, INPUT_PULLUP);
    _handler() = handler;
  }

  static void loop() {
    static unsigned long prevTime = millis();
    unsigned long now = millis();
    if (now - prevTime < DEBOUNCE_DELAY) return;

    State newState = (digitalRead<SW_B>() << 1) | digitalRead<SW_A>();
    if (newState != _state()) {
      _state() = newState;
      prevTime = now;
      if (_handler() != nullptr) _handler()(newState);
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

  static void setCoils(uint8_t index) {
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

  static void halfStep(Direction dir = Clockwise) {
    static uint8_t index = 0;
    index = (index + (dir == Clockwise ? 1 : 7)) & 7;
    setCoils(index);
  }
};

template <typename Motor, typename Switch, uint16_t ClockTeeth, uint16_t GearTeeth>
class ClockTurner {
  static_assert(ClockTeeth % GearTeeth == 0);
  static constexpr uint32_t HALFSTEPS_PER_CLOCK_REVOLUTION = 2 * (ClockTeeth / GearTeeth) * Motor::STEPS_PER_REVOLUTION;

private:
  ClockTurner() = delete;
  static Hand &_hand()        { static Hand h = HourHand; return h; }
  static bool &_initialized() { static bool i = false;    return i; }
  
public:
  static void begin() {
    Motor::begin();
    Switch::begin(+[](typename Switch::State state){
      _hand() = (state == Switch::Up)     ? SecondHand :
                (state == Switch::Middle) ? MinuteHand :
                (state == Switch::Down)   ? HourHand : HourHand;
      _initialized() = false;
    });    
  }

  static void loop() {
    static uint32_t accumulator = 0;
    static uint32_t lastTime = 0;
    static uint32_t period = 0;

    Switch::loop();

    if (!_initialized()) {
      _initialized() = true;
      lastTime = millis();
      accumulator = 0;
      period = []() {
        switch (_hand()) {
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
      Motor::halfStep();
    }
    lastTime = now;
  }
};
