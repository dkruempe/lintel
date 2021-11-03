#include "base_library/core/services/ProcessService.h"

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/services/SignalService.h"

ProcessService::ProcessService(std::shared_ptr<ProcessName> processName)
    : AbstractService<ProcessService>(processName->getProcessName()),
      m_processName(std::move(processName)),
      m_monitorThread([&] { run(); }) {}

std::future<int> ProcessService::startOf(const Process& process) {
  std::promise<void> promise;
  ProcessMap::accessor found;
  m_processes.find(found, process.getId());
  if (!found.empty()) {
    LOG_ERROR("{}/{}: process already found in list", process.getId(),
              process.getPath().filename().string());
    return found->second.getFuture();
  }

  std::filesystem::path path = process.getPath();
  if (!is_regular_file(process.getPath())) {
    auto realPath = boost::process::search_path(process.getPath().string());
    path = realPath.string();
    if (!is_regular_file(path)) {
      LOG_ERROR("{}/{}: is not a file", process.getId(),
                process.getPath().string());
      std::promise<int> p;
      p.set_value(-1);
      return p.get_future();
    }
    path = realPath.string();
  }

  ProcessMap::accessor insert;
  m_processes.insert(insert, process.getId());
  insert->second.setProcess(std::make_unique<Process>(process));
  insert->second.setChild(boost::process::child(
      path.string(), insert->second.getProcess()->getArgs(),
      boost::process::std_out > stdout, boost::process::std_err > stderr));
  insert->second.getProcess()->onStart();
  return insert->second.getFuture();
}

void ProcessService::onShutdown() {
  m_running.store(false);
  m_condition.notify_all();
}

void ProcessService::run() {
  while (m_running) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait_for(lock, std::chrono::seconds(1));
    monitor();
  }
}

void ProcessService::monitor() {
  for (auto& [name, processExecutes] : m_processes) {
    bool running = processExecutes.getChild().running();
    if (running) {
      continue;
    }
    if (processExecutes.getProcess()->getMaxAutoRestarts() != 0 &&
        processExecutes.getProcess()->currentRestarts() <
            processExecutes.getProcess()->getMaxAutoRestarts()) {
      processExecutes.getProcess()->increaseRestarts();
      std::filesystem::path path;
      if (!is_regular_file(processExecutes.getProcess()->getPath())) {
        auto tmp = boost::process::search_path(
            processExecutes.getProcess()->getPath().string());
        if (is_regular_file(tmp)) {
          path = tmp.string();
        }
      }
      processExecutes.setChild(boost::process::child(
          path.string(), processExecutes.getProcess()->getArgs(),
          boost::process::std_out > boost::process::null,
          boost::process::std_err > stderr));
      LOG_INFO("{}/{} process restarted", processExecutes.getProcess()->getId(),
               processExecutes.getProcess()->getPath().filename().string());
      processExecutes.getProcess()->onRestart();
      continue;
    }
    int exitCode = processExecutes.getChild().exit_code();
    processExecutes.setPromiseValue(exitCode);
    processExecutes.getProcess()->onStop();
    m_processes.erase(name);
  }
}

ProcessService::~ProcessService() {
  if (m_monitorThread.joinable()) {
    m_monitorThread.join();
  }
}
bool ProcessService::stopOf(const Process& process) {
  ProcessMap::accessor found;
  m_processes.find(found, process.getId());
  if (found.empty()) {
    return false;
  }
  LOG_INFO("{}/{} send kill {} SIGINT", process.getId(),
           process.getPath().filename().string(),
           found->second.getChild().id());
  SignalService::kill(found->second.getChild().id(), SIGINT);
  bool success = true;
  // only wait if process is still running
  if (found->second.getChild().running()) {
    LOG_INFO("{}/{} -> process is still running with  {}", process.getId(),
             process.getPath().filename().string(),
             found->second.getChild().id());
    std::condition_variable cond;
    std::mutex mutex;
    std::unique_lock lock(mutex);
    bool ret = cond.wait_for(
        lock, std::chrono::milliseconds(200),
        [&]() -> bool { return !found->second.getChild().running(); });
    LOG_INFO("{}/{} -> process shutdown {}", process.getId(),
             process.getPath().filename().string(), ret ? "true" : "false");
  }
  if (!found->second.getChild().running()) {
    found->second.setPromiseValue(found->second.getChild().exit_code());
    found->second.getProcess()->onStop();
  }
  if (success) {
    m_processes.erase(found);
  }
  return success;
}

/*
std::vector<Process::ProcessInfo> ProcessService::allProcesses() {
  std::vector<Process::ProcessInfo> tmp;
  std::transform(
      m_processes.begin(), m_processes.end(), std::back_inserter(tmp),
      [](const Process &process) -> Process::ProcessInfo {
        return Process::ProcessInfo{
            process.getId(),
            process.isEnabled(),
            process.getName(),
            process.getChild() != nullptr && process.getChild().running(),
            process.getChild() != nullptr && process.getChild().running()
                ? process.getChild().id()
                : 0,
            process.getExitCode()};
      });
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
  return process.getChild().running();
}

void ProcessService::terminateOf(std::size_t id) {
  Process &process = m_processes[id];
  if (process.getChild() == nullptr) {
    return;
  }
  process.getChild().terminate();
}

void ProcessService::terminateAll() {
  for (auto &process : m_processes) {
    if (process.getChild() == nullptr) {
      continue;
    }
    process.getChild().terminate();
  }
}

void ProcessService::stopOf(std::size_t id) {
  Process &process = m_processes[id];
  if (process.getChild() == nullptr || !process.getChild().running()) {
    return;
  }
  process.getChild().wait_for(m_waitTimeForShutdown);
}

void ProcessService::stopAll() {
  for (auto &process : m_processes) {
    if (process.getChild() == nullptr) {
      continue;
    }
    if (!process.getChild().running()) {
      continue;
    }
    process.getChild().wait_for(m_waitTimeForShutdown);
  }
}

void ProcessService::restartOf(std::size_t id) {
  Process &process = m_processes[id];
  if (process.getChild() != nullptr && process.getChild().running()) {
    process.getChild().wait_for(m_waitTimeForShutdown);
  }
  process.startChild();
}

void ProcessService::restartAll() {
  for (auto &process : m_processes) {
    if (process.getChild() != nullptr && process.getChild().running()) {
      process.getChild().wait_for(m_waitTimeForShutdown);
    }
    process.startChild();
  }
}

void ProcessService::detachOf(std::size_t id) {
  Process &process = m_processes[id];
  process.getChild().detach();
}

void ProcessService::enableOf(std::size_t id) {
  Process &process = m_processes[id];
  if (process.isEnabled()) {
    return;
  }
  process.setEnable(true);
}

void ProcessService::monitor() {
  for (auto &process : m_processes) {
    // check if process unexpected stopped
    if (process.getChild() != nullptr && !process.getChild().running() &&
        process.isEnabled() && process.getExitCode() == -1) {
      process.setExitCode(process.getChild().exit_code());
      checkExitCodeOf(process);
    }
    // check if restart is needed
    if (process.getChild() != nullptr && !process.getChild().running() &&
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
}*/
// Process::ProcessInfo ProcessService::ofCurrentProcess(int argc, char *argv[])
// {
//   const int processId = boost::this_process::get_id();
//   std::filesystem::path path = argv[0];
//   Process::ProcessInfo processInfo{-1,   true,      path.filename().string(),
//                                    true, processId, -1};
//   return processInfo;
// }
