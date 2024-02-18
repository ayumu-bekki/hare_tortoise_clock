#ifndef UTIL_H_
#define UTIL_H_
// ESP32 Hare Tortoise Clock
// (C)2024 bekki.jp

// Include ----------------------
#include <chrono>
#include <string>
#include <vector>

namespace HareTortoiseClockSystem::Util {

/// Sleep
void SleepMillisecond(const uint32_t sleep_milliseconds);

/// GetEpoch
std::time_t GetEpoch();

/// SetSystemTime
void SetSystemTime(const std::time_t set_epoch_time);

/// Epoch To Local Time
std::tm EpochToLocalTime(std::time_t epoch);

/// GetLocalTime
std::tm GetLocalTime();

/// Get Time To String (yyyy/dd/mm hh:mm:ss)
std::string TimeToStr(const std::tm& time_info);

/// Get Now Date String (yyyy/dd/mm hh:mm:ss)
std::string GetNowTimeStr();

/// Get Time To String (yyyy/dd/mm hh:mm:ss)
std::string TimeToStr(const std::tm& time_info);

/// Initialie Local Time Zone
void InitTimeZone();

/// Get ChronoMinutes from hours and minutes.
std::chrono::minutes GetChronoHourMinutes(const std::tm& time_info);

/// Split Text
std::vector<std::string> SplitString(const std::string& str, const char delim);

}  // namespace HareTortoiseClockSystem::Util

#endif  // UTIL_H_
