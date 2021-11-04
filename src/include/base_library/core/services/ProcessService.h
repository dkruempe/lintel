#ifndef CPP_SYSTEM_LIBRARY_PROCESSSERVICE_H
#define CPP_SYSTEM_LIBRARY_PROCESSSERVICE_H

#include <oneapi/tbb/concurrent_hash_map.h>

#include <atomic>
#include <boost/process.hpp>
#include <condition_variable>
#include <filesystem>
#include <future>
#include <memory>
#include <mutex>
#include <thread>

#include "base_library/core/services/AbstractService.h"
#include "base_library/features/base/models/Process.h"
#include "base_library/features/base/models/ProcessGroup.h"
#include "base_library/features/base/models/ProcessName.h"
#include "base_library/features/property/services/PropertyService.h"

class ProcessService : public AbstractService<ProcessService> {
 private:
  class ProcessExecutes {
   private:
    // variables
    std::unique_ptr<Process> m_process = nullptr;
    boost::process::child m_child;
    std::unique_ptr<std::promise<int>> m_promise = nullptr;

   public:
    ProcessExecutes() = default;

    void setProcess(std::unique_ptr<Process> &&process) {
      m_process = std::move(process);
    }

    void setChild(boost::process::child &&child) { m_child = std::move(child); }
    [[nodiscard]] const std::unique_ptr<Process> &getProcess() const {
      return m_process;
    }
    [[nodiscard]] boost::process::child &getChild() { return m_child; }

    void setPromiseValue(int exitCode) { m_promise->set_value(exitCode); }

    void setPromiseException(const std::exception_ptr &p) {
      m_promise->set_exception(p);
    }

    std::future<int> getFuture() {
      if (m_promise == nullptr) {
        m_promise = std::make_unique<std::promise<int>>();
      }
      return m_promise->get_future();
    }
  };

  // injections
  std::shared_ptr<ProcessName> m_processName;
  // variables
  typedef tbb::concurrent_hash_map<std::string, ProcessExecutes> ProcessMap;
  ProcessMap m_processes;
  typedef tbb::concurrent_hash_map<std::string, std::unique_ptr<ProcessGroup>>
      ProcessGroupMap;
  ProcessGroupMap m_processGroups;
  std::thread m_monitorThread;
  std::atomic_bool m_running = true;
  std::condition_variable m_condition;
  std::mutex m_mutex;
  // properties
  DEFINE_PROPERTY(m_monitorWaitTime, std::chrono::seconds,
                  std::chrono::seconds(1), "Wait time after monitor cycle",
                  true);
  DEFINE_PROPERTY(m_processStopWaitTime, std::chrono::milliseconds,
                  std::chrono::milliseconds(200),
                  "Wait time for stopping a process", true);

  void run();
  void monitorProcess();
  void monitorProcessGroups();

 public:
  explicit ProcessService(std::shared_ptr<ProcessName> processName);
  virtual ~ProcessService();

  // Process operations
  std::future<int> startOf(const Process &process);
  void restartOf(const Process &process);
  bool stopOf(const Process &process);
  void terminateOf(const Process &process);
  void detachOf(const Process &process);

  // Process group operations
  void startOf(const ProcessGroup &processGroup);
  void restartOf(const ProcessGroup &processGroup);
  bool stopOf(const ProcessGroup &processGroup);
  void terminateOf(const ProcessGroup &processGroup);
  void detachOf(const ProcessGroup &processGroup);

  // AbstractService Functions
  void onShutdown() override;
};

#endif  // CPP_SYSTEM_LIBRARY_PROCESSSERVICE_H
