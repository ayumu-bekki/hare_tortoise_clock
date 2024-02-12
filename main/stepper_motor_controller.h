#ifndef STEPPER_MOTOR_CONTROLLER_H_
#define STEPPER_MOTOR_CONTROLLER_H_
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
      : dir_(ROTATE_RIGHT), timer_tick_count_(0), step_num_(0) {}
  StepperMotorExecInfo(const RotateDir dir, const uint64_t timer_tick_count,
                       const int32_t step_num)
      : dir_(dir), timer_tick_count_(timer_tick_count), step_num_(step_num) {}

  const RotateDir dir_;
  const uint64_t timer_tick_count_;
  const int32_t step_num_;
};

/// ステッピングモーターコントロールクラス
class StepperMotorController {
 public:
  enum EventType {
    NONE = 0,
    TIMER = 1,
    INPUT_LEFT_LIMIT = 2,
    INPUT_RIGHT_LIMIT = 3,
    EMERGENCY_STOP = 4,
  };

 public:
  StepperMotorController(const uint32_t gptimer_resolution,
                         const gpio_num_t gpio_enable,
                         const gpio_num_t gpio_step, const gpio_num_t gpio_dir,
                         const gpio_num_t gpio_right_limit,
                         const gpio_num_t gpio_left_limit,
                         const bool is_rotate_right_is_dir_up);
  ~StepperMotorController();

  /// コピー禁止
  StepperMotorController(const StepperMotorController&) = delete;
  StepperMotorController& operator=(const StepperMotorController&) = delete;

  /// 緊急停止
  void EmergencyStop();

  /// モーター動作
  MoveResult ExecMove(const StepperMotorExecInfo& exec_info);
  /// モーター動作(非同期版)
  MoveResultFuture ExecMoveAsync(const StepperMotorExecInfo& exec_info);

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
};

using StepperMotorControllerSharedPtr = std::shared_ptr<StepperMotorController>;
using StepperMotorControllerWeakPtr = std::weak_ptr<StepperMotorController>;

}  // namespace RabbitClockSystem

#endif  // STEPPER_MOTOR_CONTROLLER_H_
