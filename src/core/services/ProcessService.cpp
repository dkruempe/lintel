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
      boost::process::std_out > boost::process::null,
      boost::process::std_err > boost::process::null));
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
    m_condition.wait_for(lock, m_monitorWaitTime->getValue());
    monitorProcess();
    monitorProcessGroups();
  }
}

void ProcessService::monitorProcessGroups() {
  for (const auto& [name, processGroup] : m_processGroups) {
    auto processes = processGroup->getProcesses();
    bool remove = true;
    for (const auto& process : processes) {
      ProcessMap::accessor found;
      m_processes.find(found, process.getId());
      if (!found.empty()) {
        remove = false;
      }
    }
    m_processGroups.erase(name);
  }
}

void ProcessService::monitorProcess() {
  // check state of process
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
          boost::process::std_err > boost::process::null));
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
    // process not available => stop successful bc. not available
    return true;
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
        lock, m_processStopWaitTime->getValue(),
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
void ProcessService::restartOf(const Process& process) {
  bool success = stopOf(process);
  if (!success) {
    LOG_INFO("{}/{} force stop of process", process.getId(),
             process.getPath().filename().string());
    terminateOf(process);
  }
  startOf(process);
}
void ProcessService::terminateOf(const Process& process) {
  ProcessMap::accessor found;
  m_processes.find(found, process.getId());
  if (found.empty()) {
    return;
  }
  bool running = found->second.getChild().running();
  if (running) {
    found->second.getChild().terminate();
  }
  found->second.setPromiseValue(found->second.getChild().exit_code());
  found->second.getProcess()->onTerminate();
  m_processes.erase(found);
}
void ProcessService::detachOf(const Process& process) {
  ProcessMap::accessor found;
  m_processes.find(found, process.getId());
  if (found.empty()) {
    LOG_ERROR("{}/{} process not found", process.getId(),
              process.getPath().filename().string());
    return;
  }
  // detach process => erase process from list bc. of separate running process
  // with no further monitoring
  found->second.getChild().detach();
  // inform about success exit bc. no further monitoring
  found->second.setPromiseValue(EXIT_SUCCESS);
  m_processes.erase(found);
}
void ProcessService::startOf(const ProcessGroup& processGroup) {
  ProcessGroupMap::accessor found;
  m_processGroups.find(found, processGroup.getId());
  if (!found.empty()) {
    LOG_ERROR("{}/{} processGroup exits", processGroup.getId(),
              processGroup.getName());
    return;
  }
  ProcessGroupMap::accessor insert;
  m_processGroups.insert(insert, processGroup.getId());
  insert->second = std::make_unique<ProcessGroup>(processGroup);
  for (const auto& process : insert->second->getProcesses()) {
    startOf(process);
  }
}
bool ProcessService::stopOf(const ProcessGroup& processGroup) {
  ProcessGroupMap::accessor found;
  m_processGroups.find(found, processGroup.getId());
  if (found.empty()) {
    LOG_ERROR("{}/{} processGroup doesn't exists => stop not available",
              processGroup.getId(), processGroup.getName());
    return true;
  }
  bool success = true;
  for (const auto& process : found->second->getProcesses()) {
    bool ret = stopOf(process);
    if (!ret) {
      success = false;
    }
  }
  if (success) {
    m_processGroups.erase(found);
  }
  return success;
}
void ProcessService::restartOf(const ProcessGroup& processGroup) {
  bool success = stopOf(processGroup);
  if (!success) {
    terminateOf(processGroup);
  }
  startOf(processGroup);
}
void ProcessService::terminateOf(const ProcessGroup& processGroup) {
  ProcessGroupMap::accessor found;
  m_processGroups.find(found, processGroup.getId());
  if (found.empty()) {
    LOG_ERROR("{}/{} processGroup doesn't exists => no terminate",
              processGroup.getId(), processGroup.getName());
    return;
  }
  for (const auto& process : found->second->getProcesses()) {
    terminateOf(process);
  }
  m_processGroups.erase(found);
}
void ProcessService::detachOf(const ProcessGroup& processGroup) {
  ProcessGroupMap::accessor found;
  m_processGroups.find(found, processGroup.getId());
  if (found.empty()) {
    LOG_ERROR("{}/{} processGrouop doesn't exists => no detach available");
    return;
  }
  for (const auto& process : found->second->getProcesses()) {
    detachOf(process);
  }
  m_processGroups.erase(found);
}
