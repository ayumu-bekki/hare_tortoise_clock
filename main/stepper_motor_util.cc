// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------
#include "stepper_motor_util.h"

#include <freertos/FreeRTOS.h>

namespace RabbitClockSystem::StepperMotorUtil {

/// Frequency(Hz) to Tick
uint32_t FrequencyToTick(const uint32_t hz) {
  return STEPPER_MOTOR_RESOLUTION / hz / 2;
}

/// mm to Step
int32_t MMtoStep(const uint32_t mm) {
  return static_cast<float>(mm / STEPPER_MOTOR_REVOLUTION_MOVE_MM) *
         STEPPER_MOTOR_REVOLUTION_STEP * CONFIG_STEPPER_MOTOR_STEP_DIVIDE;
}

}  // namespace RabbitClockSystem::StepperMotorUtil
