#ifndef RABBIT_CLOCK_INTERFACE_H_
#define RABBIT_CLOCK_INTERFACE_H_
// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------
#include <chrono>
#include <memory>

namespace RabbitClockSystem {

class RabbitClockInterface {
 public:
  virtual ~RabbitClockInterface() = default;

  virtual void SetUnixTime(const std::time_t epoc) = 0;
  virtual void EmergencyStop() = 0;
  virtual std::time_t GetUnixTime() const = 0;
};

using RabbitClockInterfaceSharedPtr = std::shared_ptr<RabbitClockInterface>;
using RabbitClockInterfaceConstSharedPtr =
    std::shared_ptr<const RabbitClockInterface>;
using RabbitClockInterfaceWeakPtr = std::weak_ptr<RabbitClockInterface>;
using RabbitClockInterfaceConstWeakPtr =
    std::weak_ptr<const RabbitClockInterface>;

}  // namespace RabbitClockSystem

#endif  // RABBIT_CLOCK_INTERFACE_H_
