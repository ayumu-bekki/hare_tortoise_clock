// ESP32 Hare Tortoise Clock
// (C)2024 bekki.jp

// Include ----------------------
#include "logger.h"

namespace HareTortoiseClockSystem::Logger {

void InitializeLogLevel() {
#if CONFIG_DEBUG != 0
  esp_log_level_set("*", ESP_LOG_INFO);
  esp_log_level_set(TAG, ESP_LOG_VERBOSE);
  ESP_LOGD(TAG, "DEBUG MODE");
#else
  esp_log_level_set("*", ESP_LOG_WARN);
  esp_log_level_set(TAG, ESP_LOG_INFO);
#endif
}

}  // namespace HareTortoiseClockSystem::Logger
