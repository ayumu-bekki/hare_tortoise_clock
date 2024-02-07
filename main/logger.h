#ifndef LOGGER_H_
#define LOGGER_H_
// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------
#include <esp_log.h>

// systen Loglevel Redefine
#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

namespace RabbitClockSystem {

/// Application Log Tag
static constexpr char TAG[] = "RabbitClock";

namespace Logger {

void InitializeLogLevel();

}  // namespace Logger
}  // namespace RabbitClockSystem

#endif  // LOGGER_H_
