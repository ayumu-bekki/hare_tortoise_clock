// ESP32 Hare Tortoise Clock
// (C)2024 bekki.jp

// Include ----------------------
#include "hare_tortoise_clock.h"

/// Entry Point
extern "C" void app_main() {
  std::make_shared<HareTortoiseClockSystem::HareTortoiseClock>()->Start();
}
