// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------
#include "ble_services.h"

#include <algorithm>
#include <cstring>

#include "logger.h"
#include "stepper_motor_util.h"
#include "util.h"

namespace RabbitClockSystem {

BleTimeCharacteristic::BleTimeCharacteristic(
    esp_bt_uuid_t characteristic_uuid, esp_gatt_char_prop_t property,
    const RabbitClockInterfaceWeakPtr rabbit_clock_interface)
    : BleCharacteristicInterface(),
      characteristic_uuid_(characteristic_uuid),
      property_(property),
      rabbit_clock_interface_(rabbit_clock_interface) {}

void BleTimeCharacteristic::Write(const std::vector<uint8_t> *const data) {
  RabbitClockInterfaceSharedPtr rabbit_clock = rabbit_clock_interface_.lock();
  if (!rabbit_clock) {
    return;
  }

  // [time_t型(uint64_t)] ビッグエンディアン
  ESP_LOGI(TAG, "RECV Time");
  // esp_log_buffer_hex(TAG, data->data(), data->size());
  if (data->size() == sizeof(uint64_t)) {
    // ネットワークバイトオーダー(Big-Endian)をLittle-Endianに変換
    std::vector<uint8_t> reverse(*data);
    std::reverse(reverse.begin(), reverse.end());
    const uint64_t unixtime = *reinterpret_cast<uint64_t *>(reverse.data());
    ESP_LOGI(TAG, "RECV TIME %" PRId64, unixtime);
    rabbit_clock->SetUnixTime(unixtime);
  }
}

void BleTimeCharacteristic::Read(std::vector<uint8_t> *const data) {
  RabbitClockInterfaceSharedPtr rabbit_clock = rabbit_clock_interface_.lock();
  if (!rabbit_clock) {
    return;
  }

  // [time_t型(uint64_t)] ビッグエンディアン
  std::vector<uint8_t> payload(8);
  uint64_t *const view = reinterpret_cast<uint64_t *>(payload.data());
  *view = rabbit_clock->GetUnixTime();
  std::reverse(
      payload.begin(),
      payload
          .end());  // Little-Endianをネットワークバイトオーダー(Big-Endian)に変換
  data->insert(data->end(), payload.begin(), payload.end());
}

void BleTimeCharacteristic::SetHandle(const uint16_t handle) {
  handle_ = handle;
}

uint16_t BleTimeCharacteristic::GetHandle() const { return handle_; }

esp_bt_uuid_t BleTimeCharacteristic::GetUuid() const {
  return characteristic_uuid_;
}

esp_gatt_char_prop_t BleTimeCharacteristic::GetProperty() const {
  return property_;
}

BleCommandCharacteristic::BleCommandCharacteristic(
    esp_bt_uuid_t characteristic_uuid, esp_gatt_char_prop_t property,
    const RabbitClockInterfaceWeakPtr rabbit_clock_interface)
    : BleCharacteristicInterface(),
      characteristic_uuid_(characteristic_uuid),
      property_(property),
      rabbit_clock_interface_(rabbit_clock_interface) {}

void BleCommandCharacteristic::Write(const std::vector<uint8_t> *const data) {
  esp_log_buffer_hex(TAG, data->data(), data->size());

  if (data->size() != 1) {
    return;
  }

  const uint8_t cmd = *data->data();

  if (cmd == 1) {
    ESP_LOGI(TAG, "Command 1 > System Restart");
    esp_restart();
  }
  if (cmd == 2) {
    ESP_LOGI(TAG, "Command 2 > Emergency Stop");
    RabbitClockInterfaceSharedPtr rabbit_clock = rabbit_clock_interface_.lock();
    if (!rabbit_clock) {
      return;
    }
    rabbit_clock->EmergencyStop();
  }
}

void BleCommandCharacteristic::SetHandle(const uint16_t handle) {
  handle_ = handle;
}

uint16_t BleCommandCharacteristic::GetHandle() const { return handle_; }

esp_bt_uuid_t BleCommandCharacteristic::GetUuid() const {
  return characteristic_uuid_;
}

esp_gatt_char_prop_t BleCommandCharacteristic::GetProperty() const {
  return property_;
}

BleClockService::BleClockService(const uint16_t app_id,
                                 esp_bt_uuid_t service_uuid,
                                 const uint16_t handle_num)
    : BleServiceInterface(),
      app_id_(app_id),
      gatts_if_(0),
      gatts_handle_num_(handle_num)  // 必要なハンドルの数(Attribute数)
      ,
      service_uuid_(service_uuid),
      characteristics_() {}

void BleClockService::GattsEvent(esp_gatts_cb_event_t event,
                                 esp_gatt_if_t gatts_if,
                                 esp_ble_gatts_cb_param_t *param) {
  if (event == ESP_GATTS_REG_EVT) {
    ESP_LOGI(TAG, "REGISTER_APP_EVT, status %d, app_id %d", param->reg.status,
             param->reg.app_id);

    esp_gatt_srvc_id_t service_id = {
        .id = {.uuid = service_uuid_, .inst_id = 0x00}, .is_primary = true};
    esp_ble_gatts_create_service(gatts_if, &service_id, gatts_handle_num_);

  } else if (event == ESP_GATTS_READ_EVT) {
    ESP_LOGI(TAG, "GATT_READ_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d",
             param->read.conn_id, param->read.trans_id, param->read.handle);

    for (BleCharacteristicInterfaceSharedPtr bleCharacteristic :
         characteristics_) {
      if (bleCharacteristic->GetHandle() == param->read.handle) {
        std::vector<uint8_t> data;
        bleCharacteristic->Read(&data);

        esp_gatt_rsp_t rsp = {
            .attr_value = {.value = {},
                           .handle = param->read.handle,
                           .offset = 0,
                           .len = static_cast<uint16_t>(data.size()),
                           .auth_req = 0}};
        std::memcpy(rsp.attr_value.value, data.data(), data.size());
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id,
                                    param->read.trans_id, ESP_GATT_OK, &rsp);
      }
    }

  } else if (event == ESP_GATTS_WRITE_EVT) {
    ESP_LOGI(TAG,
             "GATT_WRITE_EVT, conn_id %d, trans_id %" PRIu32
             ", handle %d, need_resp %d",
             param->write.conn_id, param->write.trans_id, param->write.handle,
             param->write.need_rsp ? 1 : 0);

    for (BleCharacteristicInterfaceSharedPtr bleCharacteristic :
         characteristics_) {
      if (bleCharacteristic->GetHandle() == param->write.handle) {
        std::vector<uint8_t> data(param->write.value,
                                  param->write.value + param->write.len);
        bleCharacteristic->Write(&data);
      }
    }

    if (param->write.need_rsp) {
      esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                  param->write.trans_id, ESP_GATT_OK, nullptr);
    }

  } else if (event == ESP_GATTS_MTU_EVT) {
    ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);

  } else if (event == ESP_GATTS_CREATE_EVT) {
    ESP_LOGI(TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d",
             param->create.status, param->create.service_handle);
    const uint16_t service_handle = param->create.service_handle;
    esp_ble_gatts_start_service(service_handle);

    for (BleCharacteristicInterfaceSharedPtr bleCharacteristic :
         characteristics_) {
      esp_bt_uuid_t uuid = bleCharacteristic->GetUuid();
      esp_err_t add_char_ret = esp_ble_gatts_add_char(
          service_handle, &uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
          bleCharacteristic->GetProperty(), nullptr, nullptr);
      if (add_char_ret) {
        ESP_LOGE(TAG, "add char failed, error code =%x", add_char_ret);
      }
    }

  } else if (event == ESP_GATTS_ADD_CHAR_EVT) {
    ESP_LOGI(TAG, "GATTS_ADD_CHAR_EVT, status %d, service_handle %d",
             param->start.status, param->start.service_handle);
    if (param->start.status != 0) {
      ESP_LOGE(TAG, "Failed Add Char Event");
      return;
    }

    for (BleCharacteristicInterfaceSharedPtr bleCharacteristic :
         characteristics_) {
      esp_bt_uuid_t uuid = bleCharacteristic->GetUuid();
      if (std::memcmp(&uuid, &param->add_char.char_uuid,
                      sizeof(param->add_char.char_uuid)) == 0) {
        ESP_LOGI(TAG, "Set Characteristic Handle, handle:%d",
                 param->start.service_handle);
        bleCharacteristic->SetHandle(param->start.service_handle);
      }
    }

  } else if (event == ESP_GATTS_START_EVT) {
    ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d",
             param->start.status, param->start.service_handle);

  } else if (event == ESP_GATTS_CONNECT_EVT) {
    ESP_LOGI(TAG,
             "ESP_GATTS_CONNECT_EVT, conn_id %d, remote "
             "%02x:%02x:%02x:%02x:%02x:%02x:",
             param->connect.conn_id, param->connect.remote_bda[0],
             param->connect.remote_bda[1], param->connect.remote_bda[2],
             param->connect.remote_bda[3], param->connect.remote_bda[4],
             param->connect.remote_bda[5]);

    esp_ble_conn_update_params_t conn_params = {
        .bda = {},        // Bluetooth device address
        .min_int = 0x10,  // Min connection interval 0x10 * 1.25ms = 20ms
        .max_int = 0x20,  // Max connection interval 0x20 * 1.25ms = 40ms
        .latency = 0,     // Slave latency for the connection in number of
                          // connection events. Range: 0x0000 to 0x01F3
        .timeout = 1000,  // Supervision timeout for the LE Link. (Range:0 -
                          // 3200) (X * 10msec) 1000 = 10s
    };
    std::memcpy(conn_params.bda, param->connect.remote_bda,
                sizeof(esp_bd_addr_t));
    esp_ble_gap_update_conn_params(&conn_params);

  } else if (event == ESP_GATTS_DISCONNECT_EVT) {
    ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x",
             param->disconnect.reason);
    BleDevice::GetInstance()->StartAdvertising();

  } else if (event == ESP_GATTS_CONF_EVT) {
    ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d",
             param->conf.status, param->conf.handle);
    if (param->conf.status != ESP_GATT_OK) {
      esp_log_buffer_hex(TAG, param->conf.value, param->conf.len);
    }

  } else if (event == ESP_GATTS_RESPONSE_EVT) {
    // ESP_LOGI(TAG, "GATTS_RESPONSE_EVT, status %d service_handle %d",
    // param->conf.status, param->start.service_handle);
  } else {
    ESP_LOGI(TAG, "EVENT %d, status %d, service_handle %d", event,
             param->start.status, param->start.service_handle);
  }
}

void BleClockService::AddCharacteristic(
    BleCharacteristicInterfaceSharedPtr bleCharacteristic) {
  characteristics_.push_back(bleCharacteristic);
}

void BleClockService::SetGattsIf(const uint16_t gatts_if) {
  gatts_if_ = gatts_if;
}

uint16_t BleClockService::GetAppId() const { return app_id_; }

uint16_t BleClockService::GetGattsIf() const { return gatts_if_; }

}  // namespace RabbitClockSystem
