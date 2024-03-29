// ESP32 Hare Tortoise Clock
// (C)2024 bekki.jp

// Include ----------------------
#include "clock_management_task.h"

#include <driver/gpio.h>
#include <driver/gptimer.h>

#include "gpio_control.h"
#include "logger.h"
#include "message_queue.h"
#include "hare_tortoise_clock_interface.h"
#include "stepper_motor_util.h"
#include "util.h"

namespace HareTortoiseClockSystem {

constexpr int32_t HALF_DAY_HOUR = 12;

// ClockMangementTask Updateタスク スリープ時間
constexpr int32_t CLOCK_MANAGEMENT_TASK_UPDATE_SLEEP_MS = 1000;

// 左から見た絶対位置
constexpr uint32_t POSITION_LEFT_RESET_MM = 0;
constexpr uint32_t POSITION_LEFT_LIMIT_MM = 10;
constexpr uint32_t POSITION_CLOCK_START_MM = 35;
constexpr uint32_t POSITION_CLOCK_END_MM = 635;
constexpr uint32_t POSITION_RIGHT_LIMIT_MM = 660;
constexpr uint32_t POSITION_RESET_MOVE_MM = 700;
constexpr uint32_t CLOCK_LENGTH_MM = 600;
constexpr uint32_t CLOCK_HOUR_MM = CLOCK_LENGTH_MM / HALF_DAY_HOUR;
constexpr uint32_t CLOCK_MINUTE_MM = CLOCK_LENGTH_MM / 60;

/// 動作スピード(周波数) (AT2100 (1/16step) MAX 20khz) ----
// ポジションリセット時
constexpr uint32_t RESET_MOVE_HZ = 3000;
// 時間設定時
constexpr uint32_t SET_TIME_MOVE_HZ = 3000;
// 通常挙動
constexpr uint32_t NORMAL_MOVE_HZ = 800;
// Minite動作速度
constexpr uint32_t MINUTE_MOVE_HZ = 800;
// Minite戻り速度
constexpr uint32_t MINUTE_RETURN_MOVE_HZ = 800;
/// Hour進み速度
constexpr uint32_t HOUR_MOVE_HZ =
    (MINUTE_RETURN_MOVE_HZ / HALF_DAY_HOUR);  // - 30; // 補正
/// Hour動作速度
constexpr uint32_t HOUR_MOVE_SLOW_HZ = 400;

const std::function<void(ClockManagementTask&)>
    ClockManagementTask::UPDATE_TASKS[MAX_CLOCK_STATUS] = {
        &ClockManagementTask::TaskDummy,       // STATUS_NONE
        &ClockManagementTask::TaskError,       // STATUS_ERROR,
        &ClockManagementTask::TaskInitialize,  // STATUS_INITIALIZE,
        &ClockManagementTask::TaskDummy,       // STATUS_SETTING_WAIT,
        &ClockManagementTask::TaskEnable,      // STATUS_ENABLE,
        &ClockManagementTask::TaskSetting,     // STATUS_SETTING,
};

ClockManagementTask::ClockManagementTask(
    const HareTortoiseClockInterfaceWeakPtr hare_tortoise_clock_interface)
    : Task(std::string(TASK_NAME).c_str(), PRIORITY, CORE_ID),
      hare_tortoise_clock_interface_(std::move(hare_tortoise_clock_interface)),
      clock_status_(STATUS_NONE),
      stepper_motor_hour_(),
      stepper_motor_minute_(),
      hour_(0),
      minute_(0),
      hour_pos_left_mm_(0),
      minute_pos_left_mm_(0) {}

void ClockManagementTask::Initialize() {
  ESP_LOGI(TAG, "Start Clock Management Task");

  // Create StepperMotorController
  stepper_motor_hour_ = std::make_shared<StepperMotorController>(
      STEPPER_MOTOR_RESOLUTION,
      static_cast<gpio_num_t>(CONFIG_HOUR_HAND_ENABLE_OUTPUT_GPIO_NO),
      static_cast<gpio_num_t>(CONFIG_HOUR_HAND_STEP_OUTPUT_GPIO_NO),
      static_cast<gpio_num_t>(CONFIG_HOUR_HAND_DIR_OUTPUT_GPIO_NO),
      static_cast<gpio_num_t>(CONFIG_HOUR_HAND_RIGHT_LIMIT_INPUT_GPIO_NO),
      static_cast<gpio_num_t>(CONFIG_HOUR_HAND_LEFT_LIMIT_INPUT_GPIO_NO),
      CONFIG_IS_STEPPER_MOTOR_ROTATE_RIGHT_IS_DIR_UP);

  stepper_motor_minute_ = std::make_shared<StepperMotorController>(
      STEPPER_MOTOR_RESOLUTION,
      static_cast<gpio_num_t>(CONFIG_MINUTE_HAND_ENABLE_OUTPUT_GPIO_NO),
      static_cast<gpio_num_t>(CONFIG_MINUTE_HAND_STEP_OUTPUT_GPIO_NO),
      static_cast<gpio_num_t>(CONFIG_MINUTE_HAND_DIR_OUTPUT_GPIO_NO),
      static_cast<gpio_num_t>(CONFIG_MINUTE_HAND_RIGHT_LIMIT_INPUT_GPIO_NO),
      static_cast<gpio_num_t>(CONFIG_MINUTE_HAND_LEFT_LIMIT_INPUT_GPIO_NO),
      CONFIG_IS_STEPPER_MOTOR_ROTATE_RIGHT_IS_DIR_UP);

  clock_status_ = STATUS_INITIALIZE;
  hour_pos_left_mm_ = 0;
  minute_pos_left_mm_ = 0;
}

void ClockManagementTask::Update() {
  if (0 < clock_status_ && clock_status_ < MAX_CLOCK_STATUS) {
    UPDATE_TASKS[clock_status_](*this);
  }
  Util::SleepMillisecond(CLOCK_MANAGEMENT_TASK_UPDATE_SLEEP_MS);
}

void ClockManagementTask::TaskDummy() {}

void ClockManagementTask::TaskInitialize() {
  ESP_LOGI(TAG, "Start Initialize ----------");

  // モーター位置をリセット
  if (!ResetAllPosition(RESET_MOVE_HZ)) {
    ESP_LOGE(TAG, "Failed Reset Position.");
    clock_status_ = STATUS_ERROR;
    return;
  }

  // 初期待機位置に移動
  ESP_LOGI(TAG, "Set Position Home");
  if (!SetBothPosition(POSITION_LEFT_LIMIT_MM, NORMAL_MOVE_HZ,
                       POSITION_LEFT_LIMIT_MM, NORMAL_MOVE_HZ)) {
    ESP_LOGE(TAG, "Failed Motor Error.");
    clock_status_ = STATUS_ERROR;
    return;
  }

  clock_status_ = STATUS_SETTING_WAIT;

  ESP_LOGI(TAG, "Finish Initialize ----------");
}

void ClockManagementTask::TaskSetting() {
  ESP_LOGI(TAG, "Start Setting ----------");

  const std::tm time_info = Util::GetLocalTime();
  hour_ = time_info.tm_hour % HALF_DAY_HOUR;
  minute_ = time_info.tm_min;

  // Monitoring LED OFF
  GPIO::SetLevel(static_cast<gpio_num_t>(CONFIG_MONITORING_OUTPUT_GPIO_NO),
                 false);

  // HourとMinuteを同時に動かす
  if (!SetBothPosition(CalcHourPos(hour_), SET_TIME_MOVE_HZ,
                       CalcMinutePos(minute_), SET_TIME_MOVE_HZ)) {
    ESP_LOGE(TAG, "Failed Motor Error.");
    clock_status_ = STATUS_ERROR;
    return;
  }
  clock_status_ = STATUS_ENABLE;

  ESP_LOGI(TAG, "Finish Setting ----------");
}

void ClockManagementTask::TaskEnable() {
  const std::tm time_info = Util::GetLocalTime();
  const std::string time_str = Util::TimeToStr(time_info);

  ESP_LOGI(TAG, "Status Enable. Now > %s", time_str.c_str());

  if ((time_info.tm_hour % HALF_DAY_HOUR) != hour_) {
    hour_ = time_info.tm_hour % HALF_DAY_HOUR;
    minute_ = 0;

    if (hour_ == 0) {
      // NEXT_12_HOUR
      Next12Hour();
    } else {
      // 59->60 HOUR
      NextHour();
    }
  } else if (time_info.tm_min != minute_) {
    minute_ = time_info.tm_min;
    if (SetMinutePosition(CalcMinutePos(minute_), MINUTE_MOVE_HZ) !=
        RESULT_STEP_FINISH) {
      ESP_LOGE(TAG, "Failed Motor Error.");
      clock_status_ = STATUS_ERROR;
    }
  }
}

void ClockManagementTask::NextHour() {
  ESP_LOGI(TAG, "Begin Next Hour ----------");

  // Minuteを右リミット位置まで進める
  if (SetMinutePosition(POSITION_RIGHT_LIMIT_MM, MINUTE_MOVE_HZ) !=
      RESULT_STEP_FINISH) {
    ESP_LOGE(TAG, "Failed Motor Error.");
    clock_status_ = STATUS_ERROR;
    return;
  }

  // Sleep (5sec)
  Util::SleepMillisecond(2000);

  // Minuteを60秒の位置まで一旦戻す(次の同時戻しと同じ速度で)
  if (SetMinutePosition(POSITION_CLOCK_END_MM, MINUTE_RETURN_MOVE_HZ) !=
      RESULT_STEP_FINISH) {
    ESP_LOGE(TAG, "Failed Motor Error.");
    clock_status_ = STATUS_ERROR;
    return;
  }

  // Hourを1時間進め、Minuteを0に戻す(同時・同時間)
  if (!SetBothPosition(CalcHourPos(hour_), HOUR_MOVE_HZ,
                       POSITION_CLOCK_START_MM, MINUTE_RETURN_MOVE_HZ)) {
    ESP_LOGE(TAG, "Failed Motor Error.");
    clock_status_ = STATUS_ERROR;
    return;
  }

  ESP_LOGI(TAG, "Finish Next Hour ----------");
}

void ClockManagementTask::Next12Hour() {
  ESP_LOGI(TAG, "Begin Next 12Hour ----------");

  // Hourをゆっくり12時間位置まで進める
  if (SetHourPosition(POSITION_CLOCK_END_MM, HOUR_MOVE_SLOW_HZ) !=
      RESULT_STEP_FINISH) {
    ESP_LOGE(TAG, "Failed Motor Error.");
    clock_status_ = STATUS_ERROR;
    return;
  }

  // Sleep (2sec)
  Util::SleepMillisecond(2000);

  // Minuteを60秒位置まで進める。
  if (SetMinutePosition(POSITION_CLOCK_END_MM, NORMAL_MOVE_HZ) !=
      RESULT_STEP_FINISH) {
    ESP_LOGE(TAG, "Failed Motor Error.");
    clock_status_ = STATUS_ERROR;
    return;
  }

  // Sleep (2sec)
  Util::SleepMillisecond(2000);

  // HourとMinuteをリセット位置に戻す
  if (!ResetAllPosition(MINUTE_RETURN_MOVE_HZ)) {
    ESP_LOGE(TAG, "Failed Reset Position.");
    clock_status_ = STATUS_ERROR;
    return;
  }

  // Sleep (1sec)
  //Util::SleepMillisecond(1000);

  // Minuteを0位置に進める
  if (SetMinutePosition(POSITION_CLOCK_START_MM, NORMAL_MOVE_HZ) !=
      RESULT_STEP_FINISH) {
    ESP_LOGE(TAG, "Failed Motor Error.");
    clock_status_ = STATUS_ERROR;
    return;
  }

  // Sleep (1sec)
  Util::SleepMillisecond(1000);

  // Hourをゆっくり0位置に進める
  if (SetHourPosition(POSITION_CLOCK_START_MM, HOUR_MOVE_SLOW_HZ) !=
      RESULT_STEP_FINISH) {
    ESP_LOGE(TAG, "Failed Motor Error.");
    clock_status_ = STATUS_ERROR;
    return;
  }

  ESP_LOGI(TAG, "Finish Next 12Hour ----------");
}

void ClockManagementTask::TaskError() {
  // Monitoring LED ON
  GPIO::SetLevel(static_cast<gpio_num_t>(CONFIG_MONITORING_OUTPUT_GPIO_NO),
                 true);
}

void ClockManagementTask::EmergencyStop() {
  ESP_LOGW(TAG, "Emergency Stop ----------");
  if (stepper_motor_hour_) {
    stepper_motor_hour_->EmergencyStop();
  }
  if (stepper_motor_minute_) {
    stepper_motor_minute_->EmergencyStop();
  }
}

MoveResult ClockManagementTask::SetHourPosition(const uint32_t position_left_mm,
                                                const uint32_t freq) {
  const int32_t move_length_mm = position_left_mm - hour_pos_left_mm_;
  const RotateDir rotate_dir =
      (0 <= move_length_mm) ? RotateDir::ROTATE_RIGHT : RotateDir::ROTATE_LEFT;

  ESP_LOGI(TAG, "Move Hour now_pos:%dmm new_hour_pos:%dmm move_len:%dmm",
           hour_pos_left_mm_, position_left_mm, move_length_mm);

  if (!stepper_motor_hour_) {
    return RESULT_ERROR;
  }
  MoveResultFuture exec_future =
      stepper_motor_hour_->ExecMoveAsync(StepperMotorExecInfo(
          rotate_dir, StepperMotorUtil::FrequencyToTick(freq),
          StepperMotorUtil::MMtoStep(std::abs(move_length_mm))));

  MoveResult move_result = exec_future.get();
  if (move_result == RESULT_STEP_FINISH) {
    hour_pos_left_mm_ = position_left_mm;
  }
  return move_result;
}

MoveResult ClockManagementTask::SetMinutePosition(
    const uint32_t position_left_mm, const uint32_t freq) {
  const int32_t move_length_mm = position_left_mm - minute_pos_left_mm_;
  const RotateDir rotate_dir =
      (0 <= move_length_mm) ? RotateDir::ROTATE_RIGHT : RotateDir::ROTATE_LEFT;

  ESP_LOGI(TAG, "Move Min now_pos:%dmm new_minute_pos:%dmm move_len:%dmm",
           minute_pos_left_mm_, position_left_mm, move_length_mm);

  if (!stepper_motor_minute_) {
    return RESULT_ERROR;
  }
  MoveResultFuture exec_future =
      stepper_motor_minute_->ExecMoveAsync(StepperMotorExecInfo(
          rotate_dir, StepperMotorUtil::FrequencyToTick(freq),
          StepperMotorUtil::MMtoStep(std::abs(move_length_mm))));

  MoveResult move_result = exec_future.get();
  if (move_result == RESULT_STEP_FINISH) {
    minute_pos_left_mm_ = position_left_mm;
  }
  return move_result;
}

bool ClockManagementTask::ResetAllPosition(const uint32_t move_hz) {
  ESP_LOGI(TAG, "Begin Reset Position");

  if (!stepper_motor_hour_ || !stepper_motor_minute_) {
    return false;
  }

  MoveResultFuture hour_reset_future = stepper_motor_hour_->ExecMoveAsync(
      StepperMotorExecInfo(RotateDir::ROTATE_LEFT,
                           StepperMotorUtil::FrequencyToTick(move_hz),
                           StepperMotorUtil::MMtoStep(POSITION_RESET_MOVE_MM)));

  MoveResultFuture minute_reset_future = stepper_motor_minute_->ExecMoveAsync(
      StepperMotorExecInfo(RotateDir::ROTATE_LEFT,
                           StepperMotorUtil::FrequencyToTick(move_hz),
                           StepperMotorUtil::MMtoStep(POSITION_RESET_MOVE_MM)));

  const MoveResult hour_reset_result = hour_reset_future.get();
  const MoveResult minute_reset_result = minute_reset_future.get();

  ESP_LOGI(TAG, "Reset Position Result Hour:%d Minute:%d", hour_reset_result,
           minute_reset_result);

  hour_pos_left_mm_ = 0;
  minute_pos_left_mm_ = 0;

  return hour_reset_result == RESULT_LEFT_LIMIT &&
         minute_reset_result == RESULT_LEFT_LIMIT;
}

bool ClockManagementTask::SetBothPosition(const uint32_t hour_pos,
                                          const uint32_t hour_hz,
                                          const uint32_t minute_pos,
                                          const uint32_t minute_hz) {
  const int32_t move_length_mm = hour_pos - hour_pos_left_mm_;
  const RotateDir rotate_dir =
      (0 <= move_length_mm) ? RotateDir::ROTATE_RIGHT : RotateDir::ROTATE_LEFT;
  ESP_LOGI(TAG, "Move Hour now_pos:%dmm new_hour_pos:%dmm move_len:%dmm",
           hour_pos_left_mm_, hour_pos, move_length_mm);
  hour_pos_left_mm_ = hour_pos;

  if (!stepper_motor_hour_) {
    return false;
  }
  MoveResultFuture hour_future =
      stepper_motor_hour_->ExecMoveAsync(StepperMotorExecInfo(
          rotate_dir, StepperMotorUtil::FrequencyToTick(hour_hz),
          StepperMotorUtil::MMtoStep(std::abs(move_length_mm))));

  MoveResult minute_result = SetMinutePosition(minute_pos, minute_hz);
  MoveResult hour_result = hour_future.get();

  return hour_result == RESULT_STEP_FINISH &&
         minute_result == RESULT_STEP_FINISH;
}

void ClockManagementTask::SetUnixTime(const std::time_t epoc) {
  // BLEスレッドから利用されるため、処理は最低限で
  if (clock_status_ == STATUS_SETTING_WAIT || clock_status_ == STATUS_ENABLE) {
    // 設定中状態
    clock_status_ = STATUS_SETTING;
    Util::SetSystemTime(epoc);
    const std::string time_str = Util::TimeToStr(Util::GetLocalTime());
    ESP_LOGI(TAG, "Set Time > %s", time_str.c_str());
  }
}

std::time_t ClockManagementTask::GetUnixTime() const {
  if (STATUS_ENABLE <= clock_status_) {
    return Util::GetEpoch();
  }
  return 0u;
}

int32_t ClockManagementTask::CalcHourPos(const int32_t hour) const {
  return POSITION_CLOCK_START_MM + ((hour_ % HALF_DAY_HOUR) * CLOCK_HOUR_MM);
}

int32_t ClockManagementTask::CalcMinutePos(const int32_t min) const {
  return POSITION_CLOCK_START_MM + (minute_ * CLOCK_MINUTE_MM);
}

}  // namespace HareTortoiseClockSystem
