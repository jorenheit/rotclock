#include "rotclock.h"

enum Pins {
  CA1  = 12,
  CA2  = 11,
  CB1  = 9,   // whoops, wired in reverse
  CB2  = 10,
  SW_A = 2,
  SW_B = 3
};

enum Params {
  CLOCK_TEETH = 216,
  GEAR_TEETH  = 36,
  MOTOR_STEPS = 96
};

using Clock = ClockTurner<
  StepperMotor<CA1, CA2, CB1, CB2, MOTOR_STEPS>,
  Switch<SW_A, SW_B>,
  CLOCK_TEETH,
  GEAR_TEETH
>;

void setup() {
  Clock::begin();
}

void loop() {
  Clock::loop();
}
