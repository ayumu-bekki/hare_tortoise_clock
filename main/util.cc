// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------
#include "util.h"

#include <esp_sntp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/err.h>
#include <lwip/sys.h>

#include <cmath>
#include <iomanip>
#include <sstream>

#include "gpio_control.h"
#include "logger.h"

namespace RabbitClockSystem {
namespace Util {

/// Sleep
void SleepMillisecond(const uint32_t sleep_milliseconds) {
  TickType_t lastWakeTime = xTaskGetTickCount();
  vTaskDelayUntil(&lastWakeTime, sleep_milliseconds / portTICK_PERIOD_MS);
}

std::time_t GetEpoch() {
  std::chrono::system_clock::time_point now_time_point =
      std::chrono::system_clock::now();
  return std::chrono::system_clock::to_time_t(now_time_point);
}

/// SetTime
void SetSystemTime(const std::time_t set_epoch_time) {
  timeval set_time;
  set_time.tv_sec = set_epoch_time;
  set_time.tv_usec = 0;
  settimeofday(&set_time, nullptr);
}

std::tm EpochToLocalTime(const std::time_t epoch) {
  return *std::localtime(&epoch);
}

std::tm GetLocalTime() { return EpochToLocalTime(GetEpoch()); }

std::string TimeToStr(const std::tm& time_info) {
  std::stringstream ss;
  ss << std::setfill('0') << std::setw(4) << (time_info.tm_year + 1900) << "/"
     << std::setw(2) << (time_info.tm_mon + 1) << "/" << std::setw(2)
     << time_info.tm_mday << " " << std::setw(2) << time_info.tm_hour << ":"
     << std::setw(2) << time_info.tm_min << ":" << std::setw(2)
     << time_info.tm_sec;
  return ss.str();
}

std::string GetNowTimeStr() { return TimeToStr(GetLocalTime()); }

void InitTimeZone() {
  setenv("TZ", CONFIG_LOCAL_TIME_ZONE, 1);
  tzset();
}

/// Get ChronoMinutes from hours and minutes.
std::chrono::minutes GetChronoHourMinutes(const std::tm& time_info) {
  return std::chrono::hours(time_info.tm_hour) +
         std::chrono::minutes(time_info.tm_min);
}

std::vector<std::string> SplitString(const std::string& str, const char delim) {
  std::vector<std::string> elements;
  std::stringstream ss(str);
  std::string item;
  while (getline(ss, item, delim)) {
    if (!item.empty()) {
      elements.push_back(item);
    }
  }
  return elements;
}

}  // namespace Util
}  // namespace RabbitClockSystem
