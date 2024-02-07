// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------
#include "ble_device.h"

#include "logger.h"

namespace RabbitClockSystem {

constexpr char DEVICE_NAME[] = "RabbitClock";
constexpr uint16_t LOCAL_MTU = 500;

static void gap_event(esp_gap_ble_cb_event_t event,
                      esp_ble_gap_cb_param_t *param) {
  BleDevice::GetInstance()->GapEvent(event, param);
}

static void gatts_event(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                        esp_ble_gatts_cb_param_t *param) {
  BleDevice::GetInstance()->GattsEvent(event, gatts_if, param);
}

BleDevice *BleDevice::this_ = nullptr;

BleDevice *BleDevice::GetInstance() {
  if (this_ == nullptr) {
    this_ = new BleDevice();
  }
  return this_;
}

BleDevice::BleDevice() : services_() {}

void BleDevice::Initialize() {
  // Initialize Bluetooth
  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  esp_err_t ret = esp_bt_controller_init(&bt_cfg);
  if (ret) {
    ESP_LOGE(TAG, "%s initialize controller failed: %s", __func__,
             esp_err_to_name(ret));
    return;
  }

  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if (ret) {
    ESP_LOGE(TAG, "%s enable controller failed: %s", __func__,
             esp_err_to_name(ret));
    return;
  }
  esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
  ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg);
  if (ret) {
    ESP_LOGE(TAG, "%s init bluetooth failed: %s", __func__,
             esp_err_to_name(ret));
    return;
  }
  ret = esp_bluedroid_enable();
  if (ret) {
    ESP_LOGE(TAG, "%s enable bluetooth failed: %s", __func__,
             esp_err_to_name(ret));
    return;
  }

  esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(DEVICE_NAME);
  if (set_dev_name_ret) {
    ESP_LOGE(TAG, "set device name failed, error code = %x", set_dev_name_ret);
    return;
  }

  // GAP
  ret = esp_ble_gap_register_callback(gap_event);
  if (ret) {
    ESP_LOGE(TAG, "gap register error, error code = %x", ret);
    return;
  }

  // Advertising
  esp_ble_adv_data_t adv_data = {
      .set_scan_rsp = false,
      .include_name = true,
      .include_txpower = false,
      .min_interval = 0x0006,  // slave connection min interval, Time =
                               // min_interval * 1.25 msec
      .max_interval = 0x0010,  // slave connection max interval, Time =
                               // max_interval * 1.25 msec
      .appearance = 0x00,
      .manufacturer_len = 0,
      .p_manufacturer_data = nullptr,
      .service_data_len = 0,
      .p_service_data = nullptr,
      .service_uuid_len = 0,
      .p_service_uuid = nullptr,
      .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
  };
  ret = esp_ble_gap_config_adv_data(&adv_data);
  if (ret) {
    ESP_LOGE(TAG, "config adv data failed, error code = %x", ret);
    return;
  }

  // GATT
  esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(LOCAL_MTU);
  if (local_mtu_ret) {
    ESP_LOGE(TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
  }

  ret = esp_ble_gatts_register_callback(gatts_event);
  if (ret) {
    ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
    return;
  }
}

void BleDevice::AddService(BleServiceInterfaceSharedPtr bleService) {
  services_.push_back(bleService);

  esp_err_t ret = esp_ble_gatts_app_register(bleService->GetAppId());
  if (ret) {
    ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
    return;
  }
}

void BleDevice::GapEvent(esp_gap_ble_cb_event_t event,
                         esp_ble_gap_cb_param_t *param) {
  if (event == ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT) {
  } else if (event == ESP_GAP_BLE_ADV_START_COMPLETE_EVT) {
    // advertising start complete event to indicate advertising start
    // successfully or failed
    if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
      ESP_LOGE(TAG, "Advertising start failed");
    }
  } else if (event == ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT) {
    if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
      ESP_LOGE(TAG, "Advertising stop failed");
    } else {
      ESP_LOGI(TAG, "Stop adv successfully");
    }
  } else if (event == ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT) {
    ESP_LOGI(
        TAG,
        "update connection params status = %d, min_int = %d, max_int = "
        "%d,conn_int = %d,latency = %d, timeout = %d",
        param->update_conn_params.status, param->update_conn_params.min_int,
        param->update_conn_params.max_int, param->update_conn_params.conn_int,
        param->update_conn_params.latency, param->update_conn_params.timeout);
  } else {
    ESP_LOGI(TAG, "GAP Event %d", event);
  }
}

void BleDevice::GattsEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                           esp_ble_gatts_cb_param_t *param) {
  if (event == ESP_GATTS_REG_EVT) {
    if (param->reg.status == ESP_GATT_OK) {
      for (BleServiceInterfaceSharedPtr bleService : services_) {
        if (bleService->GetAppId() == param->reg.app_id) {
          bleService->SetGattsIf(gatts_if);
        }
      }
    } else {
      ESP_LOGI(TAG, "Reg app failed, app_id %04x, status %d", param->reg.app_id,
               param->reg.status);
      return;
    }
  }

  for (BleServiceInterfaceSharedPtr bleService : services_) {
    if (gatts_if == ESP_GATT_IF_NONE || gatts_if == bleService->GetGattsIf()) {
      bleService->GattsEvent(event, gatts_if, param);
    }
  }
}

void BleDevice::StartAdvertising() {
  esp_ble_adv_params_t adv_params = {
      .adv_int_min = 0x0800,  // Minimum Advertising Interval (range 0x0020 -
                              // 0x4000) N * 0.625ms
      .adv_int_max = 0x0800,  // Maximum Advertising Interval (range 0x0020 -
                              // 0x4000) N * 0.625ms
      .adv_type = ADV_TYPE_IND,
      .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
      .peer_addr = {},
      .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
      .channel_map = ADV_CHNL_ALL,
      .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
  };
  esp_ble_gap_start_advertising(&adv_params);
}

}  // namespace RabbitClockSystem
