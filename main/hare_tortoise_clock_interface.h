#ifndef hare_tortoise_clock_INTERFACE_H_
#define hare_tortoise_clock_INTERFACE_H_
// ESP32 Hare Tortoise Clock
// (C)2024 bekki.jp

// Include ----------------------
#include <chrono>
#include <memory>

namespace HareTortoiseClockSystem {

class HareTortoiseClockInterface {
 public:
  virtual ~HareTortoiseClockInterface() = default;

  virtual void SetUnixTime(const std::time_t epoc) = 0;
  virtual void EmergencyStop() = 0;
  virtual std::time_t GetUnixTime() const = 0;
};

using HareTortoiseClockInterfaceSharedPtr = std::shared_ptr<HareTortoiseClockInterface>;
using HareTortoiseClockInterfaceConstSharedPtr =
    std::shared_ptr<const HareTortoiseClockInterface>;
using HareTortoiseClockInterfaceWeakPtr = std::weak_ptr<HareTortoiseClockInterface>;
using HareTortoiseClockInterfaceConstWeakPtr =
    std::weak_ptr<const HareTortoiseClockInterface>;

}  // namespace HareTortoiseClockSystem

#endif  // hare_tortoise_clock_INTERFACE_H_
