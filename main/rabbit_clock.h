#ifndef RABBIT_CLOCK_H_
#define RABBIT_CLOCK_H_
// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------
#include <memory>

#include "clock_management_task.h"
#include "rabbit_clock_interface.h"

namespace RabbitClockSystem {

/// RabbitClock
class RabbitClock final : public RabbitClockInterface,
                          public std::enable_shared_from_this<RabbitClock> {
 public:
  RabbitClock();
  ~RabbitClock();

  void Start();

  void SetUnixTime(const std::time_t epoc) override;
  void EmergencyStop() override;
  std::time_t GetUnixTime() const override;

 private:
  void CreateBLEService();

 private:
  ClockManagementSharedPtr clock_management_task_;
};

}  // namespace RabbitClockSystem

#endif  // RABBIT_CLOCK_H_
