#ifndef STEPPER_MOTOR_TASK_H_
#define STEPPER_MOTOR_TASK_H_
// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------
#include <driver/gpio.h>
#include <driver/gptimer.h>
#include <soc/soc.h>

#include <chrono>
#include <future>
#include <memory>
#include <string_view>

#include "gptimer.h"
#include "message_queue.h"
#include "task.h"

namespace RabbitClockSystem {

enum RotateDir {
  ROTATE_LEFT = 0,   // anti clockwise
  ROTATE_RIGHT = 1,  // clockwise
};

enum MoveResult {
  RESULT_NONE = 0,
  RESULT_STEP_FINISH = 1,
  RESULT_RIGHT_LIMIT = 2,
  RESULT_LEFT_LIMIT = 3,
  RESULT_ERROR = 4,
};

using MoveResultPromise = std::promise<MoveResult>;
using MoveResultFuture = std::future<MoveResult>;

class StepperMotorExecInfo {
 public:
  StepperMotorExecInfo()
      : dir_(ROTATE_RIGHT), timer_tick_count_(0), step_num_(0), promise_() {}
  StepperMotorExecInfo(const RotateDir dir, const uint64_t timer_tick_count,
                       const int32_t step_num)
      : dir_(dir),
        timer_tick_count_(timer_tick_count),
        step_num_(step_num),
        promise_() {}

  // コピーコンストラクタを削除（Promise利用のためQueueにはポインタで渡す)
  StepperMotorExecInfo(const StepperMotorExecInfo&) = delete;
  StepperMotorExecInfo& operator=(const StepperMotorExecInfo&) = delete;

  const RotateDir dir_;
  const uint64_t timer_tick_count_;
  const int32_t step_num_;
  MoveResultPromise promise_;
};

class StepperMotorTask final : public Task {
 public:
  static constexpr std::string_view TASK_NAME = "StepperMotorTask";
  static constexpr int32_t PRIORITY = Task::PRIORITY_HIGH;
  static constexpr int32_t CORE_ID = APP_CPU_NUM;

  enum EventType {
    NONE = 0,
    TIMER = 1,
    INPUT_LEFT_LIMIT = 2,
    INPUT_RIGHT_LIMIT = 3,
    EMERGENCY_STOP = 4,
  };

 public:
  StepperMotorTask(const uint32_t gptimer_resolution,
                   const gpio_num_t gpio_enable, const gpio_num_t gpio_step,
                   const gpio_num_t gpio_dir, const gpio_num_t gpio_right_limit,
                   const gpio_num_t gpio_left_limit,
                   const bool is_rotate_right_is_dir_up);

  void Initialize() override;
  void Update() override;

  void EmergencyStop();
  void AddExecInfo(StepperMotorExecInfo* const exec_info);

 private:
  MoveResult ExecMove(const StepperMotorExecInfo* const exec_info);

 public:
  static bool TimerCallback(gptimer_handle_t timer,
                            const gptimer_alarm_event_data_t* event_data,
                            void* message_queue);
  static void GpioLeftLimitCallback(void* message_queue);
  static void GpioRightLimitCallback(void* message_queue);

 private:
  const uint32_t gptimer_resolution_;
  const gpio_num_t gpio_enable_;
  const gpio_num_t gpio_step_;
  const gpio_num_t gpio_dir_;
  const gpio_num_t gpio_right_limit_;
  const gpio_num_t gpio_left_limit_;
  const bool is_rotate_right_is_dir_up_;
  MessageQueue<EventType> motor_control_queue_;
  GPTimer gptimer_;
  MessageQueue<StepperMotorExecInfo*> exec_queue_;
};

using StepperMotorTaskSharedPtr = std::shared_ptr<StepperMotorTask>;
using StepperMotorTaskWeakPtr = std::weak_ptr<StepperMotorTask>;

}  // namespace RabbitClockSystem

#endif  // STEPPER_MOTOR_TASK_H_
