#include "rotclock.h"
#include "time.h"

enum Pins {
  CA1  = 12,
  CA2  = 11,
  CB1  = 9,   // whoops, wired in reverse
  CB2  = 10,
  SW_A = 2,
  SW_B = 3,
  ERR  = 13
};

enum Params {
  CLOCK_TEETH = 216,
  GEAR_TEETH  = 36,
  MOTOR_STEPS = 96
};

enum SyncSettings {
  SyncIntervalMillis = 100,
  NudgeOnSyncMillis = 50,
  MaxAllowedDesyncSeconds = 5
};

using RTC = RTC_DS3231<0x68>;
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
  syncITC();
  Clock::loop();
}

void syncITC() {
  static uint32_t last = millis();
  uint32_t const now = millis();
  if (now - last > SyncIntervalMillis) {
    ITC::sync<RTC, NudgeOnSyncMillis, MaxAllowedDesyncSeconds>();
    last = now;
  }
}

void panic(PanicCode code) {
  Serial.begin(9600);
  pinMode(ERR, OUTPUT);
  bool ledState = HIGH; 
  while (true) {
    Clock::wobble(1000);
    Serial.print("PANIC! "); 
    Serial.println(panicCodeString(code));
    digitalWrite(ERR, ledState);
    ledState = !ledState;
  }
}