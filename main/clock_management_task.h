#ifndef CLOCK_MANAGEMENT_TASK_H_
#define CLOCK_MANAGEMENT_TASK_H_
// ESP32 Hare Tortoise Clock
// (C)2024 bekki.jp

// Include ----------------------
#include <chrono>
#include <functional>

#include "hare_tortoise_clock_interface.h"
#include "stepper_motor_controller.h"
#include "task.h"

namespace HareTortoiseClockSystem {

class ClockManagementTask final : public Task {
 public:
  static constexpr std::string_view TASK_NAME = "ClockManagementTask";
  static constexpr int32_t PRIORITY = Task::PRIORITY_LOW;
  static constexpr int32_t CORE_ID = PRO_CPU_NUM;

 private:
  enum ClockStatus {
    STATUS_NONE = 0,
    STATUS_ERROR,
    STATUS_INITIALIZE,
    STATUS_SETTING_WAIT,
    STATUS_ENABLE,
    STATUS_SETTING,
    MAX_CLOCK_STATUS,
  };

  static const std::function<void(ClockManagementTask&)>
      UPDATE_TASKS[MAX_CLOCK_STATUS];

 public:
  explicit ClockManagementTask(
      const HareTortoiseClockInterfaceWeakPtr hare_tortoise_clock_interface);

  void Initialize() override;
  void Update() override;

  void EmergencyStop();

  void SetUnixTime(const std::time_t epoc);
  std::time_t GetUnixTime() const;

 private:
  bool ResetAllPosition(const uint32_t move_hz);
  bool SetBothPosition(const uint32_t hour_pos, const uint32_t hour_hz,
                       const uint32_t minute_pos, const uint32_t minute_hz);
  MoveResult SetHourPosition(const uint32_t position_left_mm,
                             const uint32_t freq);
  MoveResult SetMinutePosition(const uint32_t position_left_mm,
                               const uint32_t freq);

  int32_t CalcHourPos(const int32_t hour) const;
  int32_t CalcMinutePos(const int32_t min) const;

  void TaskDummy();
  void TaskInitialize();
  void TaskSetting();
  void TaskEnable();
  void TaskError();

  void NextHour();
  void Next12Hour();

 private:
  const HareTortoiseClockInterfaceWeakPtr hare_tortoise_clock_interface_;
  ClockStatus clock_status_;
  StepperMotorControllerSharedPtr stepper_motor_hour_;
  StepperMotorControllerSharedPtr stepper_motor_minute_;
  int32_t hour_;
  int32_t minute_;
  int32_t hour_pos_left_mm_;
  int32_t minute_pos_left_mm_;
};

using ClockManagementSharedPtr = std::shared_ptr<ClockManagementTask>;
using ClockManagementWeakPtr = std::weak_ptr<ClockManagementTask>;

}  // namespace HareTortoiseClockSystem

#endif  // CLOCK_MANAGEMENT_TASK_H_
