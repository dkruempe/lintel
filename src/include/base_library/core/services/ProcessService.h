#ifndef CPP_SYSTEM_LIBRARY_PROCESSSERVICE_H
#define CPP_SYSTEM_LIBRARY_PROCESSSERVICE_H

#include <boost/asio/io_context.hpp>
#include <boost/process.hpp>
#include <boost/process/async.hpp>
#include <filesystem>
#include <memory>
#include <ostream>
#include <thread>
#include <utility>
#include <vector>

#include "base_library/features/base/models/Process.h"

class ProcessService {
 private:
  volatile bool m_exit = false;
  std::vector<Process> m_processes;
  std::chrono::seconds m_waitTimeForShutdown;
  std::thread m_monitorThread;
  std::chrono::milliseconds m_monitorDuration;
  static std::vector<Process> transformFunction(std::vector<Process> processes);
  void monitor();
  void run();
  static void checkExitCodeOf(Process &process);

 public:
  /**
   * constructor
   * @param processes
   * @param waitTimeForShutdown
   */
  explicit ProcessService(std::vector<Process> processes,
                          std::chrono::seconds waitTimeForShutdown,
                          std::chrono::milliseconds monitorDuration);

  ~ProcessService();

  static Process::ProcessInfo ofCurrentProcess(int argc, char *argv[]);

  /**
   * gives information about the current states of all processes
   * @return ProcessInfo
   */
  std::vector<Process::ProcessInfo> allProcesses();

  /**
   * starts given process.
   * Note: if process was initial disabled this function will enable the process
   * indirectly
   * @param nameOfProcess
   */
  void startOf(std::size_t id);
  /**
   * This method will start all processes which are enabled and not
   * automatically enable a process. For enable and start a process the method
   * startOf can be used. Alternative you can enable the process via enableOf
   */
  void startAll();
  /**
   * enables a process. So, that this process is allowed to start
   * @param nameOfProcess
   */
  void enableOf(std::size_t id);
  /**
   * checks if the process is running
   * @param nameOfProcess
   * @return true if the process is running
   */
  bool isRunning(std::size_t id);
  /**
   * aborts the running process via kill -9
   * @param nameOfProcess
   */
  void terminateOf(std::size_t id);
  /**
   * abort all running processes via kill -9
   */
  void terminateAll();
  /**
   * regular stop of process
   * @param nameOfProcess
   */
  void stopOf(std::size_t id);
  /**
   * regular stop of all processes
   */
  void stopAll();
  /**
   * regular restart of a process
   * @param nameOfProcess
   */
  void restartOf(std::size_t id);
  /**
   * regular restart of all processes
   */
  void restartAll();
  /**
   * detach a process from the master
   * @param nameOfProcess
   */
  void detachOf(std::size_t id);
};

#endif  // CPP_SYSTEM_LIBRARY_PROCESSSERVICE_H
