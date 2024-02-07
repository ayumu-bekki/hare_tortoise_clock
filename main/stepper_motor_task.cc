// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------
#include "stepper_motor_task.h"

#include <driver/gpio.h>
#include <driver/gptimer.h>

#include <future>

#include "gpio_control.h"
#include "logger.h"
#include "message_queue.h"
#include "util.h"

namespace RabbitClockSystem {

/// モータードライバーをON/OFFするインターバル時間(ms)
constexpr int32_t ENABLE_INTERVAL = 50;
/// イベントキューサイズ
constexpr int32_t EXEC_QUEUE_SIZE = 5;
/// イベントキュー最大待機tick
constexpr int32_t EXEC_QUEUE_RECEIVE_LIMIT = 1000 / portTICK_PERIOD_MS;
/// イベントキューサイズ
constexpr int32_t MOTOR_CONTROL_QUEUE_SIZE = 10;
/// イベントキュー最大待機tick
constexpr int32_t MOTOR_CONTROL_QUEUE_RECEIVE_LIMIT = 1000 / portTICK_PERIOD_MS;

StepperMotorTask::StepperMotorTask(const uint32_t gptimer_resolution,
                                   const gpio_num_t gpio_enable,
                                   const gpio_num_t gpio_step,
                                   const gpio_num_t gpio_dir,
                                   const gpio_num_t gpio_right_limit,
                                   const gpio_num_t gpio_left_limit,
                                   const bool is_rotate_right_is_dir_up)
    : Task(std::string(TASK_NAME).c_str(), PRIORITY, CORE_ID),
      gptimer_resolution_(gptimer_resolution),
      gpio_enable_(gpio_enable),
      gpio_step_(gpio_step),
      gpio_dir_(gpio_dir),
      gpio_right_limit_(gpio_right_limit),
      gpio_left_limit_(gpio_left_limit),
      is_rotate_right_is_dir_up_(is_rotate_right_is_dir_up),
      motor_control_queue_(),
      gptimer_(),
      exec_queue_() {}

void StepperMotorTask::Initialize() {
  ESP_LOGI(TAG, "Start Stepper Motor Task");

  // Init Gpio
  GPIO::InitOutput(gpio_enable_, 1);  // HIGHで無効 LOWで有効
  GPIO::InitOutput(gpio_step_, 0);
  GPIO::InitOutput(gpio_dir_, 0);  // HIGHで時計回り
  GPIO::InitInput(gpio_right_limit_);
  GPIO::InitInput(gpio_left_limit_);

  // Create MessageQueue
  if (!exec_queue_.Create(EXEC_QUEUE_SIZE)) {
    ESP_LOGE(TAG, "Creating queue failed");
  }
  if (!motor_control_queue_.Create(MOTOR_CONTROL_QUEUE_SIZE)) {
    ESP_LOGE(TAG, "Creating queue failed");
  }

  // Set Gpio Input Callback
  gpio_isr_handler_add(gpio_right_limit_,
                       &StepperMotorTask::GpioRightLimitCallback,
                       &motor_control_queue_);
  gpio_isr_handler_add(gpio_left_limit_,
                       &StepperMotorTask::GpioLeftLimitCallback,
                       &motor_control_queue_);

  // Create Timer
  gptimer_.Create(gptimer_resolution_, &StepperMotorTask::TimerCallback,
                  &motor_control_queue_);
}

void StepperMotorTask::Update() {
  StepperMotorExecInfo *exec_info = nullptr;
  while (true) {
    if (exec_queue_.ReceiveWait(&exec_info, EXEC_QUEUE_RECEIVE_LIMIT)) {
      if (exec_info) {
        exec_info->promise_.set_value(ExecMove(exec_info));
      }
    }
  }
}

void StepperMotorTask::AddExecInfo(StepperMotorExecInfo *const exec_info) {
  if (!exec_info) {
    return;
  }
  ESP_LOGI(TAG, "Add Queue dir:%d step:%d", exec_info->dir_,
           exec_info->step_num_);
  exec_queue_.Send(exec_info);
}

void StepperMotorTask::EmergencyStop() {
  ESP_LOGI(TAG, "Stepper Motor. Add Queue EmergencyStop");
  motor_control_queue_.Send(EventType::EMERGENCY_STOP);
}

MoveResult StepperMotorTask::ExecMove(
    const StepperMotorExecInfo *const exec_info) {
  if (!exec_info) {
    return RESULT_ERROR;
  }

  ESP_LOGI(TAG, "Start Exec Motor. dir:%d step:%d", exec_info->dir_,
           exec_info->step_num_);
  // リミット事前チェック
  bool is_right_on = GPIO::GetLevel(gpio_right_limit_);
  bool is_left_on = GPIO::GetLevel(gpio_left_limit_);
  if (is_right_on && exec_info->dir_ == ROTATE_RIGHT) {
    ESP_LOGW(TAG, "Motor Limit Left");
    return RESULT_RIGHT_LIMIT;
  } else if (is_left_on && exec_info->dir_ == ROTATE_LEFT) {
    ESP_LOGW(TAG, "Motor Limit Right");
    return RESULT_LEFT_LIMIT;
  }

  // モーター動作
  int32_t record = exec_info->step_num_ * 2;
  EventType event_type = EventType::NONE;
  MoveResult result = RESULT_STEP_FINISH;

  GPIO::SetLevel(gpio_enable_, 0);  // LOWで有効
  GPIO::SetLevel(gpio_step_, 0);
  GPIO::SetLevel(gpio_dir_,  // HIGHで時計回り
                 !(is_rotate_right_is_dir_up_ ^ exec_info->dir_));
  Util::SleepMillisecond(ENABLE_INTERVAL);

  gptimer_.Start(exec_info->timer_tick_count_);

  while (record) {
    if (motor_control_queue_.ReceiveWait(&event_type,
                                         MOTOR_CONTROL_QUEUE_RECEIVE_LIMIT)) {
      if (event_type == EventType::TIMER) {
        record--;
        GPIO::SetLevel(gpio_step_, record % 2);
      } else if (event_type == EventType::INPUT_RIGHT_LIMIT) {
        is_right_on = GPIO::GetLevel(gpio_right_limit_);
        // ESP_LOGD(TAG, "Right %s", is_right_on ? "ON":"OFF");
        if (is_right_on && exec_info->dir_ == ROTATE_RIGHT) {
          result = RESULT_RIGHT_LIMIT;
          break;
        }
      } else if (event_type == EventType::INPUT_LEFT_LIMIT) {
        is_left_on = GPIO::GetLevel(gpio_left_limit_);
        // ESP_LOGD(TAG, "Left %s", is_left_on ? "ON":"OFF");
        if (is_left_on && exec_info->dir_ == ROTATE_LEFT) {
          result = RESULT_LEFT_LIMIT;
          break;
        }
      } else if (event_type == EventType::EMERGENCY_STOP) {
        result = RESULT_ERROR;
        break;
      }
    } else {
      ESP_LOGE(TAG, "Missed one count event");
      result = RESULT_ERROR;
      break;
    }
  }

  gptimer_.Stop();

  GPIO::SetLevel(gpio_step_, 0);
  Util::SleepMillisecond(ENABLE_INTERVAL);
  GPIO::SetLevel(gpio_enable_, 1);

  ESP_LOGI(TAG, "Finish Motor Exec");
  return result;
}

bool IRAM_ATTR StepperMotorTask::TimerCallback(
    gptimer_handle_t timer, const gptimer_alarm_event_data_t *event_data,
    void *message_queue) {
  MessageQueue<EventType> *const queue =
      static_cast<MessageQueue<EventType> *>(message_queue);
  return queue->SendFromISR(EventType::TIMER);
}

void IRAM_ATTR StepperMotorTask::GpioLeftLimitCallback(void *message_queue) {
  MessageQueue<EventType> *const queue =
      static_cast<MessageQueue<EventType> *>(message_queue);
  queue->SendFromISR(EventType::INPUT_LEFT_LIMIT);
}

void IRAM_ATTR StepperMotorTask::GpioRightLimitCallback(void *message_queue) {
  MessageQueue<EventType> *const queue =
      static_cast<MessageQueue<EventType> *>(message_queue);
  queue->SendFromISR(EventType::INPUT_RIGHT_LIMIT);
}

}  // namespace RabbitClockSystem
