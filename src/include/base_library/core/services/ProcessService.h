#ifndef CPP_SYSTEM_LIBRARY_PROCESSSERVICE_H
#define CPP_SYSTEM_LIBRARY_PROCESSSERVICE_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <future>
#include <memory>
#include <mutex>
#include <thread>

#include "base_library/core/services/PropertyRegistration.h"
#include "base_library/core/utils/ProcessResourceReader.h"
#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/configuration/EnvironmentConfiguration.h"
#include "base_library/features/base/controller/ProcessGroupDto.h"
#include "base_library/features/base/models/Process.h"
#include "base_library/features/base/models/ProcessGroup.h"
#include "base_library/features/base/models/ProcessInfo.h"
#include "base_library/features/base/models/ProcessName.h"
#include "base_library/features/base/services/IHistoryService.h"

/** Service for managing process execution, monitoring, and process groups. */
class ProcessService : public PropertyRegistration<ProcessService>
{
private:
  /** Internal wrapper holding a running process, its child handle, and an exit-code promise. */
  class ProcessExecutes
  {
  private:
    // variables
    std::shared_ptr<Process> m_process = nullptr;
    std::shared_ptr<boost::process::v1::child> m_child;
    std::shared_ptr<std::promise<int>> m_promise = nullptr;
    // automation state
    std::chrono::steady_clock::time_point m_processStartTime{};
    std::chrono::steady_clock::time_point m_lastRestartTime{};
    int m_consecutiveFailures = 0;
    bool m_failed = false;
    std::optional<ProcessResourceData> m_lastResourceSample;
    std::chrono::steady_clock::time_point m_lastNotifyTime{};

  public:
    ProcessExecutes() = default;

    /** Set the process descriptor.
     * @param process the process to associate */
    void setProcess(std::shared_ptr<Process> process) { m_process = std::move(process); }

    /** Set the child process handle.
     * @param child the child process handle */
    void setChild(std::shared_ptr<boost::process::v1::child> child)
    {
      m_child = std::move(child);
      m_processStartTime = std::chrono::steady_clock::now();
    }

    /** Returns the associated process descriptor.
     * @return the process descriptor */
    [[nodiscard]] const std::shared_ptr<Process> &getProcess() const { return m_process; }

    /** Returns the child process handle.
     * @return the child process handle */
    [[nodiscard]] std::shared_ptr<boost::process::v1::child> &getChild() { return m_child; }

    /** Set the promise value with the given exit code.
     * @param exitCode the process exit code */
    void setPromiseValue(int exitCode)
    {
      if (m_promise == nullptr) { m_promise = std::make_shared<std::promise<int>>(); }
      m_promise->set_value(exitCode);
    }

    /** Set the promise exception.
     * @param p the exception pointer */
    void setPromiseException(const std::exception_ptr &p)
    {
      if (m_promise == nullptr) { m_promise = std::make_shared<std::promise<int>>(); }
      m_promise->set_exception(p);
    }

    /** Get a future that will be fulfilled with the process exit code.
     * @return a future for the exit code */
    std::future<int> getFuture()
    {
      if (m_promise == nullptr) { m_promise = std::make_shared<std::promise<int>>(); }
      return m_promise->get_future();
    }

    /** @param time the time at which the current child was spawned */
    void markStart() { m_processStartTime = std::chrono::steady_clock::now(); }

    /** @return the time at which the current child was spawned */
    [[nodiscard]] std::chrono::steady_clock::time_point processStartTime() const { return m_processStartTime; }

    /** @param time the time of the most recent auto-restart */
    void markRestart() { m_lastRestartTime = std::chrono::steady_clock::now(); }

    /** @return the time of the most recent auto-restart */
    [[nodiscard]] std::chrono::steady_clock::time_point lastRestartTime() const { return m_lastRestartTime; }

    /** @param failures the consecutive-failure count */
    void setConsecutiveFailures(int failures) { m_consecutiveFailures = failures; }

    /** @return the consecutive-failure count */
    [[nodiscard]] int consecutiveFailures() const { return m_consecutiveFailures; }

    /** @param failed the circuit-breaker flag */
    void setFailed(bool failed) { m_failed = failed; }

    /** @return true if the process has been moved to the failed state */
    [[nodiscard]] bool isFailed() const { return m_failed; }

    /** @param sample the most recent resource snapshot */
    void setResourceSample(ProcessResourceData sample) { m_lastResourceSample = sample; }

    /** @return the most recent resource snapshot */
    [[nodiscard]] const std::optional<ProcessResourceData> &resourceSample() const { return m_lastResourceSample; }

    /** @param time the time of the most recent threshold-notify */
    void setLastNotifyTime(std::chrono::steady_clock::time_point time) { m_lastNotifyTime = time; }

    /** @return the time of the most recent threshold-notify */
    [[nodiscard]] std::chrono::steady_clock::time_point lastNotifyTime() const { return m_lastNotifyTime; }
  };

  // injections
  std::shared_ptr<ProcessName> m_processName;
  std::shared_ptr<EnvironmentConfiguration> m_environmentConfiguration;
  std::shared_ptr<Process> m_process;
  std::shared_ptr<Configuration> m_configuration;
  std::shared_ptr<IHistoryService> m_historyService;
  // variables
  std::mutex m_processesMutex;
  std::map<std::string, ProcessExecutes> m_processes;
  std::mutex m_processGroupMutex;
  std::map<std::string, std::shared_ptr<ProcessGroup>> m_processGroups;
  std::thread m_monitorThread;
  std::atomic_bool m_running = true;
  std::condition_variable m_condition;
  std::mutex m_mutex;
  // properties
  std::shared_ptr<Property<std::chrono::seconds>> m_monitorWaitTime;
  std::shared_ptr<Property<std::chrono::milliseconds>> m_processStopWaitTime;

  void run();

  void monitorProcess();

  void monitorProcessGroups();

  /** Resolve an executable path (direct file or via environment configuration).
   * @param path the configured path
   * @return the resolved path */
  std::filesystem::path resolvePath(const std::filesystem::path &path) const;

  /** Spawn a child process with the configured bootstrap config.
   * @param path the resolved executable path
   * @param args the command-line arguments
   * @param configName the bootstrap config name for the child, empty to inherit
   * @return the child process handle */
  std::shared_ptr<boost::process::v1::child>
    spawnChild(const std::filesystem::path &path, const std::vector<std::string> &args, const std::string &configName);

  /** Sample and attach resource data for a tracked process, triggering notify
   * thresholds if configured.
   * @param id the tracked process id
   * @param processExecutes the tracked process entry */
  void sampleResources(const std::string &id, ProcessExecutes &processExecutes);

  /** Refresh the cached resource sample of a running process if it is missing
   * or stale, so health/CLI reads return current data even before the monitor
   * thread's periodic tick.
   * @param processExecutes the tracked process entry */
  void refreshResourceSampleIfStale(ProcessExecutes &processExecutes);

  /** Read the tracked process into a ProcessInfo (safe exit code + resource data).
   * @param id the tracked process id
   * @param processExecutes the tracked process entry
   * @param groupName the group name (or "none")
   * @param groupId the group id (or "none") */
  ProcessInfo processInfoOf(const std::string &id,
    ProcessExecutes &processExecutes,
    const std::string &groupName,
    const std::string &groupId);

public:
  /** Construct a ProcessService.
   * @param processName              the process name
   * @param environmentConfiguration the environment configuration
   * @param configuration            the application configuration
   * @param historyService           the process history service */
  ProcessService(std::shared_ptr<ProcessName> processName,
    std::shared_ptr<EnvironmentConfiguration> environmentConfiguration,
    std::shared_ptr<Configuration> configuration,
    std::shared_ptr<IHistoryService> historyService);

  ~ProcessService() override;

  // Process Information
  /** Returns all process groups matching the given name.
   * @param name the group name to search for
   * @return list of matching process group DTOs */
  std::vector<ProcessGroupDto> allGroupsOf(const std::string &name);

  /** Returns information for all active processes.
   * @return list of active process info */
  std::vector<ProcessInfo> allActiveOf();

  /** Returns the current process information.
   * @return the current process info */
  ProcessInfo currentOf();

  /** Returns the process with the given ID, if it exists.
   * @param id the process ID
   * @return the process, or std::nullopt if not found */
  std::optional<std::shared_ptr<Process>> of(const std::string &id);

  // Process operations
  /** Start a process asynchronously.
   * @param process the process to start
   * @return a future for the exit code, or std::nullopt if start failed */
  std::optional<std::future<int>> startOf(const Process &process);

  /** Restart a running process.
   * @param process the process to restart */
  void restartOf(const Process &process);

  /** Stop a running process gracefully.
   * @param process the process to stop
   * @return true if the stop signal was sent */
  bool stopOf(const Process &process);

  /** Terminate a running process forcefully.
   * @param process the process to terminate */
  void terminateOf(const Process &process);

  /** Detach from a running process without stopping it.
   * @param process the process to detach */
  void detachOf(const Process &process);

  /** Reset the restart counter and failure state of a process.
   * @param process the process to reset */
  void resetOf(const Process &process);

  // Process group operations
  /** Start all processes in the given group.
   * @param processGroup the group to start */
  void startOf(const ProcessGroup &processGroup);

  /** Restart all processes in the given group.
   * @param processGroup the group to restart */
  void restartOf(const ProcessGroup &processGroup);

  /** Stop all processes in the given group gracefully.
   * @param processGroup the group to stop
   * @return true if stop signals were sent */
  bool stopOf(const ProcessGroup &processGroup);

  /** Terminate all processes in the given group forcefully.
   * @param processGroup the group to terminate */
  void terminateOf(const ProcessGroup &processGroup);

  /** Detach from all processes in the given group.
   * @param processGroup the group to detach */
  void detachOf(const ProcessGroup &processGroup);

  /** Check if this is the last running process in the application.
   * @return true if this is the last process */
  bool isLastProcess();

  ShutdownPriority shutdownPriorityOf() const override { return ShutdownPriority::PROCESS_SERVICE; }

  void onInitialize() override;
};

#endif// CPP_SYSTEM_LIBRARY_PROCESSSERVICE_H
