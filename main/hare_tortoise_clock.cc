// ESP32 Hare Tortoise Clock
// (C)2024 bekki.jp

// Include ----------------------
#include "hare_tortoise_clock.h"

#include <driver/gpio.h>
#include <esp_system.h>
#include <nvs_flash.h>

#include <cstring>
#include <memory>

#include "gpio_control.h"
#include "logger.h"
#include "util.h"
#include "version.h"
#include "ble_device.h"
#include "ble_services.h"

namespace HareTortoiseClockSystem {

HareTortoiseClock::HareTortoiseClock() : clock_management_task_() {}

HareTortoiseClock::~HareTortoiseClock() = default;

void HareTortoiseClock::Start() {
  // Initialize Log
  Logger::InitializeLogLevel();

  ESP_LOGI(TAG, "Startup Hare Tortoise Clock. Version:%s", std::string(GIT_VERSION).c_str());

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

  // Bluetooth GATT Server
  CreateBLEService();

  // ClockManagementTask
  clock_management_task_ =
      std::make_shared<ClockManagementTask>(weak_from_this());
  clock_management_task_->Start();

  ESP_LOGI(TAG, "Activation Complete Hare Tortoise Clock System.");

  while (true) {
    Util::SleepMillisecond(1000);
  }
}

void HareTortoiseClock::CreateBLEService() {
  // Create BleTimeCharacteristic 157c64df-ca4b-4647-b26b-4ddc2ab42797
  constexpr esp_gatt_char_prop_t time_char_property =
      ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
  constexpr uint8_t TIME_CHARACTERISTIC_UUID_RAW[ESP_UUID_LEN_128] = {
      0x97, 0x27, 0xb4, 0x2a, 0xdc, 0x4d, 0x6b, 0xb2,
      0x47, 0x46, 0x4b, 0xca, 0xdf, 0x64, 0x7c, 0x15};
  esp_bt_uuid_t time_characteristic_uuid = {.len = ESP_UUID_LEN_128,
                                            .uuid = {.uuid128 = {}}};
  std::memcpy(time_characteristic_uuid.uuid.uuid128,
              TIME_CHARACTERISTIC_UUID_RAW, ESP_UUID_LEN_128);
  BleCharacteristicInterfaceSharedPtr ble_time_characteristic =
      std::make_shared<BleTimeCharacteristic>(
          time_characteristic_uuid, time_char_property, weak_from_this());

  // Create BleCommandCharacteristic bd902d82-f4bd-45c8-baf8-040b3d877abe
  constexpr esp_gatt_char_prop_t command_char_property =
      ESP_GATT_CHAR_PROP_BIT_WRITE;
  constexpr uint8_t DEMO_CHARACTERISTIC_UUID_RAW[ESP_UUID_LEN_128] = {
      0xbe, 0x7a, 0x87, 0x3d, 0x0b, 0x04, 0xf8, 0xba,
      0xc8, 0x45, 0xbd, 0xf4, 0x82, 0x2d, 0x90, 0xbd};
  esp_bt_uuid_t command_characteristic_uuid = {.len = ESP_UUID_LEN_128,
                                               .uuid = {.uuid128 = {}}};
  std::memcpy(command_characteristic_uuid.uuid.uuid128,
              DEMO_CHARACTERISTIC_UUID_RAW, ESP_UUID_LEN_128);
  BleCharacteristicInterfaceSharedPtr ble_command_characteristic =
      std::make_shared<BleCommandCharacteristic>(
          command_characteristic_uuid, command_char_property, weak_from_this());

  // Create BleClockService f5c85862-dd4b-4874-9089-3b9e8bcb7099
  constexpr uint8_t SERVICE_UUID_RAW[ESP_UUID_LEN_128] = {
      0x99, 0x70, 0xcb, 0x8b, 0x9e, 0x3b, 0x89, 0x90,
      0x74, 0x48, 0x4b, 0xdd, 0x62, 0x58, 0xc8, 0xf5};
  esp_bt_uuid_t service_uuid = {.len = ESP_UUID_LEN_128,
                                .uuid = {.uuid128 = {}}};

  std::memcpy(service_uuid.uuid.uuid128, SERVICE_UUID_RAW, ESP_UUID_LEN_128);
  BleServiceInterfaceSharedPtr ble_clock_service =
      std::make_shared<BleClockService>(0, service_uuid, 8);
  ble_clock_service->AddCharacteristic(ble_time_characteristic);
  ble_clock_service->AddCharacteristic(ble_command_characteristic);

  // Start Bletooth Low Energy
  BleDevice *const ble_device = BleDevice::GetInstance();
  ble_device->Initialize();
  ble_device->AddService(ble_clock_service);
  ble_device->StartAdvertising();
}

void HareTortoiseClock::SetUnixTime(const std::time_t epoc) {
  if (clock_management_task_) {
    clock_management_task_->SetUnixTime(epoc);
  }
}

void HareTortoiseClock::EmergencyStop() {
  if (clock_management_task_) {
    clock_management_task_->EmergencyStop();
  }
}

std::time_t HareTortoiseClock::GetUnixTime() const {
  if (clock_management_task_) {
    return clock_management_task_->GetUnixTime();
  }
  return 0;
}

}  // namespace HareTortoiseClockSystem
