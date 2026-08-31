#ifndef CPP_SYSTEM_LIBRARY_PROCESS_H
#define CPP_SYSTEM_LIBRARY_PROCESS_H

#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base_library/core/utils/UUID.h"

/**
 * Represents a managed process with lifecycle callbacks, auto-restart capability
 * and configurable automation policy (backoff, rate limit, periodic restart,
 * active time window and resource notify thresholds).
 */
class Process
{
private:
  // UUID
  std::string m_id = UUID::generate();
  // process
  std::filesystem::path m_path;
  std::vector<std::string> m_args;
  // configuration
  std::string m_configName;
  bool m_autoRestart = false;
  int32_t m_restarts = 0;
  int m_maxAutoRestarts = -1;
  // automation policy
  std::chrono::milliseconds m_restartDelay = std::chrono::milliseconds(0);
  std::chrono::milliseconds m_restartDelayMax = std::chrono::milliseconds(0);
  std::chrono::milliseconds m_minUptime = std::chrono::milliseconds(0);
  int m_maxRestartRate = -1;
  std::chrono::milliseconds m_restartWindow = std::chrono::milliseconds(0);
  std::chrono::milliseconds m_restartInterval = std::chrono::milliseconds(0);
  std::optional<int32_t> m_activeFromHour;
  std::optional<int32_t> m_activeToHour;
  std::optional<double> m_cpuNotify;
  std::optional<uint64_t> m_memoryNotify;
  // events
  std::shared_ptr<std::function<void(const Process &)>> m_onStart = nullptr;
  std::shared_ptr<std::function<void(const Process &)>> m_onStop = nullptr;
  std::shared_ptr<std::function<void(const Process &)>> m_onRestart = nullptr;
  std::shared_ptr<std::function<void(const Process &)>> m_onTerminate = nullptr;
  std::shared_ptr<std::function<void(const Process &)>> m_onFinish = nullptr;

public:
  /**
   * Constructor.
   * @param path path to the executable
   * @param args command-line arguments
   * @param configName bootstrap config name used by the child process (empty = inherit)
   */
  explicit Process(std::filesystem::path path, std::vector<std::string> args, std::string configName = "");

  /** @param onStart callback invoked when the process starts */
  void addOnStartEvent(std::shared_ptr<std::function<void(const Process &)>> onStart);

  /** @param onStop callback invoked when the process stops */
  void addOnStopEvent(std::shared_ptr<std::function<void(const Process &)>> onStop);

  /** @param onRestart callback invoked when the process restarts */
  void addOnRestartEvent(std::shared_ptr<std::function<void(const Process &)>> onRestart);

  /** @param onTerminate callback invoked when the process is terminated */
  void addOnTerminateEvent(std::shared_ptr<std::function<void(const Process &)>> onTerminate);

  /** @param onFinish callback invoked when the process finishes */
  void addOnFinishEvent(std::shared_ptr<std::function<void(const Process &)>> onFinish);

  /**
   * Enable auto-restart for the process.
   * @param maxAutoRestarts maximum number of restarts (-1 for unlimited)
   */
  void enableAutoStart(int maxAutoRestarts = -1);

  /** Disable auto-restart for the process. */
  void disableAutoStart();

  /** Increment the restart counter. */
  void increaseRestarts();

  /** Reset the restart counter to zero. */
  void resetRestarts();

  /** @return current number of restarts */
  [[nodiscard]] int currentRestarts() const;

  /** @return process UUID */
  [[nodiscard]] const std::string &getId() const;

  /** @return path to the executable */
  [[nodiscard]] const std::filesystem::path &getPath() const;

  /** @return true if auto-restart is enabled */
  [[nodiscard]] bool isAutoRestart() const;

  /** @return maximum number of auto-restarts */
  [[nodiscard]] int getMaxAutoRestarts() const;

  /** @return command-line arguments */
  [[nodiscard]] const std::vector<std::string> &getArgs() const;

  /** @return bootstrap config name of the child process (empty = inherit) */
  [[nodiscard]] const std::string &getConfigName() const;

  // --- automation policy ---

  /** @param delay base backoff delay before the next auto-restart (0 = immediate) */
  void setRestartDelay(std::chrono::milliseconds delay);

  /** @return base backoff delay before the next auto-restart */
  [[nodiscard]] std::chrono::milliseconds getRestartDelay() const;

  /** @param delayMax maximum (capped) backoff delay (0 = no cap) */
  void setRestartDelayMax(std::chrono::milliseconds delayMax);

  /** @return maximum (capped) backoff delay */
  [[nodiscard]] std::chrono::milliseconds getRestartDelayMax() const;

  /** @param minUptime minimum uptime in the restart window for a crash to count against the rate limit */
  void setMinUptime(std::chrono::milliseconds minUptime);

  /** @return minimum uptime to consider a crash healthy (0 = always count) */
  [[nodiscard]] std::chrono::milliseconds getMinUptime() const;

  /** @param maxRestartRate maximum restart count within the rate window (-1 = unlimited) */
  void setMaxRestartRate(int maxRestartRate);

  /** @return maximum restart count within the rate window */
  [[nodiscard]] int getMaxRestartRate() const;

  /** @param restartWindow time window over which the restart rate is evaluated */
  void setRestartWindow(std::chrono::milliseconds restartWindow);

  /** @return time window over which the restart rate is evaluated */
  [[nodiscard]] std::chrono::milliseconds getRestartWindow() const;

  /** @param restartInterval periodic restart interval (0 = disabled) */
  void setRestartInterval(std::chrono::milliseconds restartInterval);

  /** @return periodic restart interval (0 = disabled) */
  [[nodiscard]] std::chrono::milliseconds getRestartInterval() const;

  /** @param activeFromHour active window start hour (0-23), disabled if nullopt */
  void setActiveFromHour(std::optional<int32_t> activeFromHour);

  /** @return active window start hour (0-23), nullopt if disabled */
  [[nodiscard]] std::optional<int32_t> getActiveFromHour() const;

  /** @param activeToHour active window end hour (0-23), disabled if nullopt */
  void setActiveToHour(std::optional<int32_t> activeToHour);

  /** @return active window end hour (0-23), nullopt if disabled */
  [[nodiscard]] std::optional<int32_t> getActiveToHour() const;

  /** @return true if an active time window is configured */
  [[nodiscard]] bool hasActiveWindow() const;

  /** @return true if the current time is inside the configured active window */
  [[nodiscard]] bool inActiveWindow() const;

  /** @param cpuNotify CPU usage percentage threshold that triggers a notify */
  void setCpuNotify(std::optional<double> cpuNotify);

  /** @return CPU usage percentage threshold that triggers a notify */
  [[nodiscard]] std::optional<double> getCpuNotify() const;

  /** @param memoryNotify memory usage threshold in bytes that triggers a notify */
  void setMemoryNotify(std::optional<uint64_t> memoryNotify);

  /** @return memory usage threshold in bytes that triggers a notify */
  [[nodiscard]] std::optional<uint64_t> getMemoryNotify() const;

  /** Trigger the onStart event. */
  void onStart() const;

  /** Trigger the onStop event. */
  void onStop() const;

  /** Trigger the onRestart event. */
  void onRestart() const;

  /** Trigger the onTerminate event. */
  void onTerminate() const;

  /** Trigger the onFinish event. */
  void onFinish() const;
};

#endif// CPP_SYSTEM_LIBRARY_PROCESS_H
