#ifndef hare_tortoise_clock_H_
#define hare_tortoise_clock_H_
// ESP32 Hare Tortoise Clock
// (C)2024 bekki.jp

// Include ----------------------
#include <memory>

#include "clock_management_task.h"
#include "hare_tortoise_clock_interface.h"

namespace HareTortoiseClockSystem {

/// HareTortoiseClock
class HareTortoiseClock final : public HareTortoiseClockInterface,
                          public std::enable_shared_from_this<HareTortoiseClock> {
 public:
  HareTortoiseClock();
  ~HareTortoiseClock();

  void Start();

  void SetUnixTime(const std::time_t epoc) override;
  void EmergencyStop() override;
  std::time_t GetUnixTime() const override;

 private:
  void CreateBLEService();

 private:
  ClockManagementSharedPtr clock_management_task_;
};

}  // namespace HareTortoiseClockSystem

#endif  // hare_tortoise_clock_H_
