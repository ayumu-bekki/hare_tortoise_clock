#ifndef CLOCK_STATUS_H_
#define CLOCK_STATUS_H_
// ESP32 Rabbit Clock
// (C)2024 bekki.jp

namespace RabbitClockSystem {

enum ClockStatus {
  STATUS_NONE = 0,
  STATUS_ERROR,
  STATUS_INITIALIZE,
  STATUS_SETTING_WAIT,
  STATUS_ENABLE,
  STATUS_SETTING,
  STATUS_NEXT_HOUR,
  STATUS_NEXT_12HOUR,
};

}  // namespace RabbitClockSystem

#endif  // CLOCK_STATUS_H_
