#ifndef CLOCK_MANAGEMENT_TASK_H_
#define CLOCK_MANAGEMENT_TASK_H_
// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------

#include <chrono>

#include "clock_status.h"
#include "rabbit_clock_interface.h"
#include "stepper_motor_task.h"
#include "task.h"

#include <freertos/FreeRTOS.h>

namespace RabbitClockSystem {

class ClockManagementTask final : public Task {
 public:
  static constexpr std::string_view TASK_NAME = "ClockManagementTask";
  static constexpr int32_t PRIORITY = Task::PRIORITY_LOW;
  static constexpr int32_t CORE_ID = PRO_CPU_NUM;

 public:
  explicit ClockManagementTask(
      const RabbitClockInterfaceWeakPtr rabbit_clock_interface);

  void Initialize() override;
  void Update() override;

  void EmergencyStop();

  void SetUnixTime(const std::time_t epoc);
  std::time_t GetUnixTime() const;

 private:
  bool ResetAllPosition();
  bool SetBothPosition(const uint32_t hour_pos, const uint32_t hour_hz,
                       const uint32_t minute_pos, const uint32_t minute_hz);
  MoveResult SetHourPosition(const uint32_t position_left_mm,
                             const uint32_t freq);
  MoveResult SetMinutePosition(const uint32_t position_left_mm,
                               const uint32_t freq);

  int32_t CalcHourPos(const int32_t hour) const;
  int32_t CalcMinutePos(const int32_t min) const;

  void NextHour();
  void Next12Hour();

 private:
  const RabbitClockInterfaceWeakPtr rabbit_clock_interface_;
  ClockStatus clock_status_;
  StepperMotorTaskSharedPtr stepper_motor_hour_;
  StepperMotorTaskSharedPtr stepper_motor_minute_;
  int32_t hour_;
  int32_t minute_;
  int32_t hour_pos_left_mm_;
  int32_t minute_pos_left_mm_;
};

using ClockManagementSharedPtr = std::shared_ptr<ClockManagementTask>;
using ClockManagementWeakPtr = std::weak_ptr<ClockManagementTask>;

}  // namespace RabbitClockSystem

#endif  // CLOCK_MANAGEMENT_TASK_H_
