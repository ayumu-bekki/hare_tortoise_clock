// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------
#include "rabbit_clock.h"

/// Entry Point
extern "C" void app_main() {
  std::make_shared<RabbitClockSystem::RabbitClock>()->Start();
}
