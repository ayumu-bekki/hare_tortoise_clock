#ifndef GPIO_H_
#define GPIO_H_
// ESP32 Hare Tortoise Clock
// (C)2024 bekki.jp

// Include ----------------------
#include <driver/gpio.h>
#include <stdint.h>

namespace HareTortoiseClockSystem::GPIO {

/// Init GPIO ISR Service
void InitGpioIsrService();

/// Reset GPIO 
void Reset(const gpio_num_t gpio_number);

/// Init GPIO (Output)
void InitOutput(const gpio_num_t gpio_number, const bool level = false);

/// Init GPIO (Input:Pulldown inner enable)
void InitInput(const gpio_num_t gpio_number);

/// Set GPIO Level (Output)
void SetLevel(const gpio_num_t gpio_number, const bool level);

/// Gett GPIO Level (Input)
bool GetLevel(const gpio_num_t gpio_number);

}  // namespace HareTortoiseClockSystem::GPIO

#endif  // GPIO_H_
