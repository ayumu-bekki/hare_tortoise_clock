#ifndef GPIO_H_
#define GPIO_H_
// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------
#include <driver/gpio.h>
#include <stdint.h>

namespace RabbitClockSystem::GPIO {

/// Init GPIO ISR Service
void InitGpioIsrService();

/// Init GPIO (Output)
void InitOutput(const gpio_num_t gpio_number, const int32_t level = 0);

/// Init GPIO (Input:Pulldown inner enable)
void InitInput(const gpio_num_t gpio_number);

/// Set GPIO Level (Output)
void SetLevel(const gpio_num_t gpio_number, const int32_t level);

/// Gett GPIO Level (Input)
bool GetLevel(const gpio_num_t gpio_number);

}  // namespace RabbitClockSystem::GPIO

#endif  // GPIO_H_
