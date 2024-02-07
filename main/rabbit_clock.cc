// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------
#include "rabbit_clock.h"

#include <driver/gpio.h>
#include <esp_system.h>
#include <nvs_flash.h>

#include <cstring>
#include <memory>

#include "gpio_control.h"
#include "logger.h"
#include "util.h"
// #include "version.h"
#include "ble_service_task.h"

namespace RabbitClockSystem {

RabbitClock::RabbitClock() : clock_management_task_() {}

RabbitClock::~RabbitClock() = default;

void RabbitClock::Start() {
  // Initialize Log
  Logger::InitializeLogLevel();

  ESP_LOGI(TAG, "Startup Rabbit Clock");

  // Monitoring LED Init And ON
  GPIO::InitOutput(static_cast<gpio_num_t>(CONFIG_MONITORING_OUTPUT_GPIO_NO),
                   1);

  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGE(TAG, "NVS Flash Error");
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Init GPIO ISR Service
  GPIO::InitGpioIsrService();

  // Timezone init
  Util::InitTimeZone();

  // Create Bluetooth Service Task
  BleServiceTaskSharedPtr ble_service_task = std::make_shared<BleServiceTask>(weak_from_this());
  ble_service_task->Start();

  // Create Clock Management Tasks
  clock_management_task_ =
      std::make_shared<ClockManagementTask>(weak_from_this());
  clock_management_task_->Start();

  ESP_LOGI(TAG, "Activation Complete Rabbit Clock System.");

  while (true) {
    Util::SleepMillisecond(1000);
  }
}

void RabbitClock::SetUnixTime(const std::time_t epoc) {
  if (clock_management_task_) {
    clock_management_task_->SetUnixTime(epoc);
  }
}

void RabbitClock::EmergencyStop() {
  if (clock_management_task_) {
    clock_management_task_->EmergencyStop();
  }
}

std::time_t RabbitClock::GetUnixTime() const {
  if (clock_management_task_) {
    return clock_management_task_->GetUnixTime();
  }
  return 0;
}

}  // namespace RabbitClockSystem
