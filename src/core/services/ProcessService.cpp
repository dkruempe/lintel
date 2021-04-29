#include "base_library/core/services/ProcessService.h"

#include <algorithm>
#include <boost/process/environment.hpp>

ProcessService::ProcessService(std::vector<Process> processes,
                               std::chrono::seconds waitTimeForShutdown,
                               std::chrono::milliseconds monitorDuration)
    : m_processes(transformFunction(std::move(processes))),
      m_waitTimeForShutdown(waitTimeForShutdown),
      m_monitorThread([&] { run(); }),
      m_monitorDuration(monitorDuration) {}
ProcessService::~ProcessService() {
  m_exit = true;
  m_monitorThread.join();
  monitor();
}
void ProcessService::startOf(std::size_t id) {
  Process &process = m_processes[id];
  if (!process.isEnabled()) {
    return;
  }
  process.startChild();
}
std::vector<Process::ProcessInfo> ProcessService::allProcesses() {
  std::vector<Process::ProcessInfo> tmp;
  std::transform(
      m_processes.begin(), m_processes.end(), std::back_inserter(tmp),
      [](const Process &process) -> Process::ProcessInfo {
        return Process::ProcessInfo{
            process.getId(),
            process.isEnabled(),
            process.getName(),
            process.getChild() != nullptr && process.getChild()->running(),
            process.getChild() != nullptr && process.getChild()->running()
                ? process.getChild()->id()
                : 0,
            process.getExitCode()};
      });
  //    return Process::ProcessInfo{
  //        .id{process.getId()},
  //        .enabled = process.isEnabled(),
  //        .name = process.getName(),
  //        .running = process.getChild() != nullptr &&
  //                   process.getChild()->running(),
  //        .processId = process.getChild() != nullptr &&
  //                             process.getChild()->running()
  //                         ? process.getChild()->id()
  //                         : 0,
  //        .exitCode = process.getExitCode()};
  //  });
  return tmp;
}
void ProcessService::startAll() {
  for (auto &process : m_processes) {
    if (process.getChild() != nullptr || !process.isEnabled()) {
      // process started ignore
      continue;
    }
    process.startChild();
  }
}
bool ProcessService::isRunning(std::size_t id) {
  Process &process = m_processes[id];
  if (process.getChild() == nullptr) {
    return false;
  }
  return process.getChild()->running();
}
void ProcessService::terminateOf(std::size_t id) {
  Process &process = m_processes[id];
  if (process.getChild() == nullptr) {
    return;
  }
  process.getChild()->terminate();
}
void ProcessService::terminateAll() {
  for (auto &process : m_processes) {
    if (process.getChild() == nullptr) {
      continue;
    }
    process.getChild()->terminate();
  }
}
void ProcessService::stopOf(std::size_t id) {
  Process &process = m_processes[id];
  if (process.getChild() == nullptr || !process.getChild()->running()) {
    return;
  }
  process.getChild()->wait_for(m_waitTimeForShutdown);
}
void ProcessService::stopAll() {
  for (auto &process : m_processes) {
    if (process.getChild() == nullptr) {
      continue;
    }
    if (!process.getChild()->running()) {
      continue;
    }
    process.getChild()->wait_for(m_waitTimeForShutdown);
  }
}
void ProcessService::restartOf(std::size_t id) {
  Process &process = m_processes[id];
  if (process.getChild() != nullptr && process.getChild()->running()) {
    process.getChild()->wait_for(m_waitTimeForShutdown);
  }
  process.startChild();
}
void ProcessService::restartAll() {
  for (auto &process : m_processes) {
    if (process.getChild() != nullptr && process.getChild()->running()) {
      process.getChild()->wait_for(m_waitTimeForShutdown);
    }
    process.startChild();
  }
}
std::vector<Process> ProcessService::transformFunction(
    std::vector<Process> processes) {
  std::sort(processes.begin(), processes.end(), Process::ProcessComparator());
  int64_t id = 0;
  for (auto &iter : processes) {
    iter.setId(id);
    id++;
  }
  return processes;
}
void ProcessService::detachOf(std::size_t id) {
  Process &process = m_processes[id];
  process.getChild()->detach();
}
void ProcessService::enableOf(std::size_t id) {
  Process &process = m_processes[id];
  if (process.isEnabled()) {
    return;
  }
  process.setEnable(true);
}

void ProcessService::run() {
  do {
    std::this_thread::sleep_for(m_monitorDuration);
    monitor();
  } while (!m_exit);
}

void ProcessService::monitor() {
  for (auto &process : m_processes) {
    // check if process unexpected stopped
    if (process.getChild() != nullptr && !process.getChild()->running() &&
        process.isEnabled() && process.getExitCode() == -1) {
      process.setExitCode(process.getChild()->exit_code());
      checkExitCodeOf(process);
    }
    // check if restart is needed
    if (process.getChild() != nullptr && !process.getChild()->running() &&
        process.isEnabled() && process.isAutomaticRestart() &&
        (process.getMaxRestarts() > process.getRestarts() ||
         process.getMaxRestarts() == -1)) {
      process.startChild();
      process.setRestarts(process.getRestarts() + 1);
    }
  }
}
void ProcessService::checkExitCodeOf(Process &process) {
  int exitCode = process.getExitCode();
  const std::string &name = process.getName();
  switch (exitCode) {
    case EXIT_SUCCESS:
      // all went fine
      break;
    case SIGQUIT:
      fprintf(stderr, "%s: ERROR quit\n", name.c_str());
      break;
    case SIGILL:
      fprintf(stderr, "%s: ERROR illegal instruction (not reset when caught)\n",
              name.c_str());
      break;
    case SIGABRT:
      fprintf(stderr, "%s: ERROR abort()\n", name.c_str());
      break;
    case SIGFPE:
      fprintf(stderr, "%s: ERROR floating point exception\n", name.c_str());
      break;
    case SIGSEGV:
      fprintf(stderr, "%s: ERROR segmentation violation\n", name.c_str());
      break;
    case SIGTERM:
      fprintf(stderr, "%s ERROR software termination signal from kill\n",
              name.c_str());
      break;
    default:
      fprintf(stderr, "%s shutdown with exit code %d\n", name.c_str(),
              exitCode);
      break;
  }
}
Process::ProcessInfo ProcessService::ofCurrentProcess(int argc, char *argv[]) {
  const int processId = boost::this_process::get_id();
  std::filesystem::path path = argv[0];
  Process::ProcessInfo processInfo{-1,   true,      path.filename().string(),
                                   true, processId, -1};
  return processInfo;
}
