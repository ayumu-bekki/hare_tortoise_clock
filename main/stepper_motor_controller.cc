// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------
#include "stepper_motor_controller.h"

#include <driver/gpio.h>
#include <driver/gptimer.h>
#include <esp_pthread.h>

#include <future>

#include "gpio_control.h"
#include "logger.h"
#include "message_queue.h"
#include "util.h"

namespace RabbitClockSystem {

/// モータードライバーをON/OFFするインターバル時間(ms) ステッピングモータードライバによって調整
constexpr int32_t ENABLE_INTERVAL = 20;
/// イベントキューサイズ
constexpr int32_t MOTOR_CONTROL_QUEUE_SIZE = 10;
/// イベントキュー最大待機tick
constexpr int32_t MOTOR_CONTROL_QUEUE_RECEIVE_LIMIT = 1000 / portTICK_PERIOD_MS;
/// 非同期実行時のスレッド名
constexpr std::string_view TASK_NAME = "StepperMotorTask";
/// 非同期実行時のスレッド利用CPUコア
constexpr int32_t CORE_ID = APP_CPU_NUM;

StepperMotorController::StepperMotorController(
    const uint32_t gptimer_resolution, const gpio_num_t gpio_enable,
    const gpio_num_t gpio_step, const gpio_num_t gpio_dir,
    const gpio_num_t gpio_right_limit, const gpio_num_t gpio_left_limit,
    const bool is_rotate_right_is_dir_up)
    : gptimer_resolution_(gptimer_resolution),
      gpio_enable_(gpio_enable),
      gpio_step_(gpio_step),
      gpio_dir_(gpio_dir),
      gpio_right_limit_(gpio_right_limit),
      gpio_left_limit_(gpio_left_limit),
      is_rotate_right_is_dir_up_(is_rotate_right_is_dir_up),
      motor_control_queue_(),
      gptimer_() {
  // Init Gpio
  GPIO::InitOutput(gpio_enable_, 1);  // HIGHで無効 LOWで有効
  GPIO::InitOutput(gpio_step_, 0);
  GPIO::InitOutput(gpio_dir_, 0);  // HIGHで時計回り
  GPIO::InitInput(gpio_right_limit_);
  GPIO::InitInput(gpio_left_limit_);

  // Create MessageQueue
  if (!motor_control_queue_.Create(MOTOR_CONTROL_QUEUE_SIZE)) {
    ESP_LOGE(TAG, "Creating queue failed");
  }

  // Set Gpio Input Callback
  gpio_isr_handler_add(gpio_right_limit_,
                       &StepperMotorController::GpioRightLimitCallback,
                       &motor_control_queue_);
  gpio_isr_handler_add(gpio_left_limit_,
                       &StepperMotorController::GpioLeftLimitCallback,
                       &motor_control_queue_);

  // Create Timer
  gptimer_.Create(gptimer_resolution_, &StepperMotorController::TimerCallback,
                  &motor_control_queue_);
}

StepperMotorController::~StepperMotorController()
{
  gptimer_.Destroy();

  motor_control_queue_.Destroy();

  gpio_isr_handler_remove(gpio_right_limit_);
  gpio_isr_handler_remove(gpio_left_limit_);
  
  GPIO::Reset(gpio_enable_);
  GPIO::Reset(gpio_step_);
  GPIO::Reset(gpio_dir_);
  GPIO::Reset(gpio_right_limit_);
  GPIO::Reset(gpio_left_limit_);
}

void StepperMotorController::EmergencyStop() {
  ESP_LOGI(TAG, "Stepper Motor. Add Queue EmergencyStop");
  motor_control_queue_.Send(EventType::EMERGENCY_STOP);
}

MoveResultFuture StepperMotorController::ExecMoveAsync(
    const StepperMotorExecInfo &exec_info) {
  // 新規スレッドを作る際の設定(コアを指定)
  esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
  cfg.thread_name = std::string(TASK_NAME).c_str();
  cfg.pin_to_core = CORE_ID;
  esp_pthread_set_cfg(&cfg);

  return std::async(std::launch::async, [this, exec_info] { return ExecMove(exec_info); });
}

MoveResult StepperMotorController::ExecMove(
    const StepperMotorExecInfo &exec_info) {
  ESP_LOGI(TAG, "Start Exec Motor. dir:%d step:%d", exec_info.dir_,
           exec_info.step_num_);
  // リミット事前チェック
  bool is_right_on = GPIO::GetLevel(gpio_right_limit_);
  bool is_left_on = GPIO::GetLevel(gpio_left_limit_);
  if (is_right_on && exec_info.dir_ == ROTATE_RIGHT) {
    ESP_LOGI(TAG, "Motor Limit Left");
    return RESULT_RIGHT_LIMIT;
  } else if (is_left_on && exec_info.dir_ == ROTATE_LEFT) {
    ESP_LOGI(TAG, "Motor Limit Right");
    return RESULT_LEFT_LIMIT;
  }

  // モーター動作
  int32_t record = exec_info.step_num_ * 2; // LOW/HIGHで1周期にするためループ回数を2倍にする(2回で1周期)
  EventType event_type = EventType::NONE;
  MoveResult result = RESULT_STEP_FINISH;

  GPIO::SetLevel(gpio_enable_, false);  // LOWで有効
  GPIO::SetLevel(gpio_step_, false);
  GPIO::SetLevel(gpio_dir_,  // HIGHで時計回り
                 !(is_rotate_right_is_dir_up_ ^ exec_info.dir_));
  Util::SleepMillisecond(ENABLE_INTERVAL);

  gptimer_.Start(exec_info.timer_tick_count_);

  while (record) {
    if (motor_control_queue_.ReceiveWait(&event_type,
                                         MOTOR_CONTROL_QUEUE_RECEIVE_LIMIT)) {
      if (event_type == EventType::TIMER) {
        record--;
        GPIO::SetLevel(gpio_step_, record % 2);
      } else if (event_type == EventType::INPUT_RIGHT_LIMIT) {
        is_right_on = GPIO::GetLevel(gpio_right_limit_);
        ESP_LOGD(TAG, "Right %s", is_right_on ? "ON":"OFF");
        if (is_right_on && exec_info.dir_ == ROTATE_RIGHT) {
          result = RESULT_RIGHT_LIMIT;
          break;
        }
      } else if (event_type == EventType::INPUT_LEFT_LIMIT) {
        is_left_on = GPIO::GetLevel(gpio_left_limit_);
        ESP_LOGD(TAG, "Left %s", is_left_on ? "ON":"OFF");
        if (is_left_on && exec_info.dir_ == ROTATE_LEFT) {
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

  Util::SleepMillisecond(ENABLE_INTERVAL);
  GPIO::SetLevel(gpio_step_, false);
  GPIO::SetLevel(gpio_enable_, true);

  ESP_LOGI(TAG, "Finish Exec Motor");
  return result;
}

bool IRAM_ATTR StepperMotorController::TimerCallback(
    gptimer_handle_t timer, const gptimer_alarm_event_data_t *event_data,
    void *message_queue) {
  MessageQueue<EventType> *const queue =
      static_cast<MessageQueue<EventType> *>(message_queue);
  return queue->SendFromISR(EventType::TIMER);
}

void IRAM_ATTR
StepperMotorController::GpioLeftLimitCallback(void *message_queue) {
  MessageQueue<EventType> *const queue =
      static_cast<MessageQueue<EventType> *>(message_queue);
  queue->SendFromISR(EventType::INPUT_LEFT_LIMIT);
}

void IRAM_ATTR
StepperMotorController::GpioRightLimitCallback(void *message_queue) {
  MessageQueue<EventType> *const queue =
      static_cast<MessageQueue<EventType> *>(message_queue);
  queue->SendFromISR(EventType::INPUT_RIGHT_LIMIT);
}

}  // namespace RabbitClockSystem
