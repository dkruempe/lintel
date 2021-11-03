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
#include "base_library/features/base/models/ProcessName.h"

class ProcessService : AbstractService<ProcessService> {
 private:
  class ProcessExecutes {
   private:
    // variables
    std::unique_ptr<Process> m_process = nullptr;
    boost::process::child m_child;
    std::promise<int> m_promise;

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

    void setPromiseValue(int exitCode) { m_promise.set_value(exitCode); }

    void setPromiseException(const std::exception_ptr &p) {
      m_promise.set_exception(p);
    }

    std::future<int> getFuture() { return m_promise.get_future(); }
  };

  // injections
  std::shared_ptr<ProcessName> m_processName;
  // variables
  typedef tbb::concurrent_hash_map<std::string, ProcessExecutes> ProcessMap;
  ProcessMap m_processes;
  std::thread m_monitorThread;
  std::atomic_bool m_running = true;
  std::condition_variable m_condition;
  std::mutex m_mutex;

  void run();
  void monitor();
  //  static void checkExitCodeOf(Process &process);

 public:
  explicit ProcessService(std::shared_ptr<ProcessName> processName);
  virtual ~ProcessService();

  std::future<int> startOf(const Process &process);
  bool stopOf(const Process &process);

  void onShutdown() override;

  // static Process::ProcessInfo ofCurrentProcess(int argc, char *argv[]);

  //  std::vector<Process::ProcessInfo> allProcesses();
  //  void startOf(std::size_t id);
  //  void startAll();
  //  void enableOf(std::size_t id);
  //  bool isRunning(std::size_t id);
  //  void terminateOf(std::size_t id);
  //  void terminateAll();
  //  void stopOf(std::size_t id);
  //  void stopAll();
  //  void restartOf(std::size_t id);
  //  void restartAll();
  //  void detachOf(std::size_t id);
};

#endif  // CPP_SYSTEM_LIBRARY_PROCESSSERVICE_H
