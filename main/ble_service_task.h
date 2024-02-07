#ifndef BLE_SERVICE_TASK_H_
#define BLE_SERVICE_TASK_H_
// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------
#include <freertos/FreeRTOS.h>

#include <chrono>
#include <memory>

#include "rabbit_clock_interface.h"
#include "task.h"

namespace RabbitClockSystem {

class BleServiceTask final : public Task {
 public:
  static constexpr std::string_view TASK_NAME = "BleServiceTask";
  static constexpr int32_t PRIORITY = Task::PRIORITY_NORMAL;
  static constexpr int32_t CORE_ID = PRO_CPU_NUM;

 public:
  explicit BleServiceTask(
      const RabbitClockInterfaceWeakPtr rabbit_clock_interface);

  void Initialize() override;
  void Update() override;

 private:
  const RabbitClockInterfaceWeakPtr rabbit_clock_interface_;
};

using BleServiceTaskSharedPtr = std::shared_ptr<BleServiceTask>;

}  // namespace RabbitClockSystem

#endif  // CLOCK_MANAGEMENT_TASK_H_
