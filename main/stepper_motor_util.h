#ifndef STEPPER_MOTOR_UTIL_H_
#define STEPPER_MOTOR_UTIL_H_
// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------
#include <cstdint>

/// 1MHz, 1 tick=1us
constexpr uint32_t STEPPER_MOTOR_RESOLUTION = 1000000u;
/// 1回転のステップ数
constexpr uint32_t STEPPER_MOTOR_REVOLUTION_STEP = 200;
/// 1回転の動作量(mm)
constexpr float STEPPER_MOTOR_REVOLUTION_MOVE_MM = 40.0f;

namespace RabbitClockSystem::StepperMotorUtil {

/// Frequency(Hz) to Tick
uint32_t FrequencyToTick(const uint32_t hz);

/// mm to Step
int32_t MMtoStep(const uint32_t mm);

}  // namespace RabbitClockSystem::StepperMotorUtil

#endif  // STEPPER_MOTOR_UTIL_H_