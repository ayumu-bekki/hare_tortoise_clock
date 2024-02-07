#ifndef BLE_SERVICES_H_
#define BLE_SERVICES_H_
// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------
#include <esp_bt.h>
#include <esp_bt_defs.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_gatt_common_api.h>
#include <esp_gatts_api.h>

#include <vector>

#include "ble_device.h"
#include "rabbit_clock_interface.h"

namespace RabbitClockSystem {

class BleTimeCharacteristic final : public BleCharacteristicInterface {
 public:
  BleTimeCharacteristic(
      esp_bt_uuid_t characteristic_uuid, esp_gatt_char_prop_t property,
      const RabbitClockInterfaceWeakPtr rabbit_clock_interface);

  void Write(const std::vector<uint8_t> *const data) override;
  void Read(std::vector<uint8_t> *const data) override;

  void SetHandle(const uint16_t handle) override;
  uint16_t GetHandle() const override;
  esp_bt_uuid_t GetUuid() const override;
  esp_gatt_char_prop_t GetProperty() const override;

 private:
  const esp_bt_uuid_t characteristic_uuid_;
  const esp_gatt_char_prop_t property_;
  uint16_t handle_;
  const RabbitClockInterfaceWeakPtr rabbit_clock_interface_;
};

class BleCommandCharacteristic final : public BleCharacteristicInterface {
 public:
  BleCommandCharacteristic(
      esp_bt_uuid_t characteristic_uuid, esp_gatt_char_prop_t property,
      const RabbitClockInterfaceWeakPtr rabbit_clock_interface);

  void Write(const std::vector<uint8_t> *const data) override;
  void Read(std::vector<uint8_t> *const data) override {}

  void SetHandle(const uint16_t handle) override;
  uint16_t GetHandle() const override;
  esp_bt_uuid_t GetUuid() const override;
  esp_gatt_char_prop_t GetProperty() const override;

 private:
  const esp_bt_uuid_t characteristic_uuid_;
  const esp_gatt_char_prop_t property_;
  uint16_t handle_;
  const RabbitClockInterfaceWeakPtr rabbit_clock_interface_;
};

class BleClockService final : public BleServiceInterface {
 public:
  BleClockService(const uint16_t app_id, esp_bt_uuid_t service_uuid,
                  const uint16_t handle_num);

 private:
  void GattsEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                  esp_ble_gatts_cb_param_t *param) override;
  void AddCharacteristic(
      BleCharacteristicInterfaceSharedPtr bleCharacteristic) override;

  void SetGattsIf(const uint16_t gatts_if) override;
  uint16_t GetAppId() const override;
  uint16_t GetGattsIf() const override;

 private:
  const uint16_t app_id_;
  uint16_t gatts_if_;
  uint16_t gatts_handle_num_;
  esp_bt_uuid_t service_uuid_;
  std::vector<BleCharacteristicInterfaceSharedPtr> characteristics_;
};

}  // namespace RabbitClockSystem

#endif  // BLE_SERVICE_DUMMY_H_
