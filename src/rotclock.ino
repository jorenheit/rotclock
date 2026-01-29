#include "exec_every.h"
#include "rotclock.h"
#include "time.h"

enum Pins {
  CA1  = 12,
  CA2  = 11,
  CB1  = 9,   // whoops, wired in reverse
  CB2  = 10,
  SW_A = 2,
  SW_B = 3
};

enum Params {
  CLOCK_TEETH       = 216,
  GEAR_TEETH        = 36,
  MOTOR_STEPS       = 96
};

enum SyncSettings {
  SyncIntervalMillis = 100,
  NudgeOnSyncMillis = 50,
  MaxAllowedDesyncSeconds = 5
};

using RTC = time::RTC_DS3231<0x68>;
using ITC = time::ITC<NudgeOnSyncMillis, MaxAllowedDesyncSeconds>;

using Clock = ClockTurner<
  StepperMotor<CA1, CA2, CB1, CB2, MOTOR_STEPS>,
  Switch<SW_A, SW_B>,
  CLOCK_TEETH,
  GEAR_TEETH,
  ITC::millis
>;

void setup() {
  Clock::begin();
  RTC::begin();
  ITC::begin();
  ITC::set(RTC::time());
}

void loop() {
  Clock::loop();
  exec_every(SyncIntervalMillis, ITC::sync<RTC>);
}

void panic(PanicCode code) {
  while (true) Clock::wobble(code, 1000);
}