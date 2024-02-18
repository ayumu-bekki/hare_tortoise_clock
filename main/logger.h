#ifndef LOGGER_H_
#define LOGGER_H_
// ESP32 Hare Tortoise Clock
// (C)2024 bekki.jp

// Include ----------------------
#include <esp_log.h>

// systen Loglevel Redefine
#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

namespace HareTortoiseClockSystem {

/// Application Log Tag
static constexpr char TAG[] = "HareTortoiseClock";

namespace Logger {

void InitializeLogLevel();

}  // namespace Logger
}  // namespace HareTortoiseClockSystem

#endif  // LOGGER_H_
