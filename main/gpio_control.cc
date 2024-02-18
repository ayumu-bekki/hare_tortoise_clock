// ESP32 Hare Tortoise Clock
// (C)2024 bekki.jp

// Include ----------------------
#include "gpio_control.h"

#include <driver/gpio.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_adc/adc_oneshot.h>

#include "logger.h"

namespace HareTortoiseClockSystem::GPIO {

/// Init GPIO ISR Service
void InitGpioIsrService() { gpio_install_isr_service(0); }

/// Reset GPIO 
void Reset(const gpio_num_t gpio_number) {
  gpio_reset_pin(gpio_number);
}

/// Init GPIO (Output)
void InitOutput(const gpio_num_t gpio_number, const bool level) {
  gpio_config_t io_conf_dir = {
      .pin_bit_mask = 1ull << gpio_number,
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&io_conf_dir);

  SetLevel(gpio_number, level);
}

/// Init GPIO (Input:Pulldown inner enable)
void InitInput(const gpio_num_t gpio_number) {
  // Input
  gpio_config_t io_input_conf = {
      .pin_bit_mask = 1ull << gpio_number,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en =
          GPIO_PULLUP_ENABLE,  // IO34～IO39は内部プルアップ/プルダウン抵抗は無し
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_ANYEDGE,
  };
  gpio_config(&io_input_conf);
}

/// Set GPIO Level (Output)
void SetLevel(const gpio_num_t gpio_number, const bool level) {
  gpio_set_level(gpio_number, level);
}

/// Gett GPIO Level (Input)
bool GetLevel(const gpio_num_t gpio_number) {
  return gpio_get_level(gpio_number) != 0;
}

}  // namespace HareTortoiseClockSystem::GPIO
