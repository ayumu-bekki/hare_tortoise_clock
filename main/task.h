#ifndef TASK_H_
#define TASK_H_
// ESP32 Rabbit Clock
// (C)2024 bekki.jp

// Include ----------------------
#include <string>

namespace RabbitClockSystem {

/// FreeRTOS xTask Wrap
class Task {
 public:
  enum TaskStatus {
    TASK_STATUS_READY,
    TASK_STATUS_RUN,
    TASK_STATUS_END,
  };

  static constexpr int32_t TASK_STAC_DEPTH = 8192;

  /// Task Priority
  static constexpr int32_t PRIORITY_LOW = 0;
  static constexpr int32_t PRIORITY_NORMAL = 1;
  static constexpr int32_t PRIORITY_HIGH = 2;

 private:
  Task();

 public:
  Task(const std::string& taskName, const int32_t priority, const int coreId);
  virtual ~Task();

  /// Start Task
  void Start();

  /// Stop Task
  void Stop();

  /// Initialize (Called when the Start function is executed.)
  virtual void Initialize() {}

  /// (override) sub class processing
  virtual void Update() = 0;

 public:
  /// Task Running
  void Run();

  /// Task Listener
  static void Listener(void* const pParam);

 protected:
  /// Task Status
  TaskStatus m_Status;

  /// Task Name
  std::string m_TaskName;

  /// Task Priority
  int32_t m_Priority;

  /// Use Core Id
  int32_t m_CoreId;
};

}  // namespace RabbitClockSystem

#endif  // TASK_H_