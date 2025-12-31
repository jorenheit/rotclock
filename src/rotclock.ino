#include "rotclock.h"

enum Pins {
  CA1  = 12,
  CA2  = 11,
  CB1  = 9,
  CB2  = 10,
  SW_A = 2,
  SW_B = 3
};

using Motor = StepperMotor<CA1, CA2, CB1, CB2>;
using Clock = ClockTurner<Motor>;
using ModeSwitch = Switch<SW_A, SW_B>;

Clock clock;
ModeSwitch modeSwitch;

void setup() {
  clock.begin();
  modeSwitch.begin([](ModeSwitch::State state){
    clock.setStationaryHand(
      (state == ModeSwitch::Up)     ? Clock::SecondHand :
      (state == ModeSwitch::Middle) ? Clock::MinuteHand :
      (state == ModeSwitch::Down)   ? Clock::HourHand : Clock::HourHand
    );
  });
}

void loop() {
  modeSwitch.loop();
  clock.loop();
}
