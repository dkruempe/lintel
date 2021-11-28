#include "base_library/core/services/ProcessService.h"

#include <base_library/features/base/models/ProcessGroup.h>
#include <base_library/features/base/models/ProcessInfo.h>

#include <boost/process/detail/child_decl.hpp>
#include <boost/process/io.hpp>
#include <memory>
#include <mutex>
#include <stdexcept>

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/services/SignalService.h"

ProcessService::ProcessService(
    std::shared_ptr<ProcessName> processName,
    std::shared_ptr<EnvironmentConfiguration> environmentConfiguration)
    : AbstractService<ProcessService>(processName->getProcessName()),
      m_processName(std::move(processName)),
      m_environmentConfiguration(std::move(environmentConfiguration)) {
  // make sure that all variables are initialized for starting the thread
  m_monitorThread = std::thread([&] { run(); });
  m_process = std::make_shared<Process>(m_processName->getPath(),
                                        m_processName->getArgs());
}
std::vector<ProcessInfo> ProcessService::allActiveOf() {
  std::map<std::string, std::shared_ptr<ProcessGroup>> processGroupMap;
  {
    std::lock_guard<std::mutex> locker(m_processGroupMutex);
    for (auto& [id, processGroup] : m_processGroups) {
      for (const auto& process : processGroup->getProcesses()) {
        processGroupMap.insert({process.getId(), processGroup});
      }
    }
  }
  std::vector<ProcessInfo> temp;
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    for (auto& [id, process] : m_processes) {
      auto found = processGroupMap.find(id);
      if (found != processGroupMap.end()) {
        temp.emplace_back(process.getProcess(), process.getChild()->id(),
                          process.getChild()->running(),
                          process.getChild()->exit_code(),
                          found->second->getName(), found->second->getId());
      } else {
        temp.emplace_back(process.getProcess(), process.getChild()->id(),
                          process.getChild()->running(),
                          process.getChild()->exit_code(), "none", "none");
      }
    }
  }
  temp.push_back(currentOf());
  return temp;
}

std::future<int> ProcessService::startOf(const Process& process) {
  // I check if process is available
  {
    std::lock_guard<std::mutex> lock(m_processesMutex);
    auto found = m_processes.find(process.getId());
    if (found != m_processes.end()) {
      throw std::runtime_error(process.getId() + "/" +
                               process.getPath().filename().string() +
                               ": process already found in list");
    }
  }

  // II check path of process
  std::filesystem::path path = process.getPath();
  if (!is_regular_file(process.getPath())) {
    std::string tmp = process.getPath().string();
    auto optPath = m_environmentConfiguration->pathOf(tmp);
    if (!optPath.has_value()) {
      throw std::runtime_error(process.getId() + "/" +
                               process.getPath().filename().string() +
                               ": is not a file");
    }
    path = optPath.value();
  }

  // III insert process
  std::future<int> future;
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    ProcessExecutes processExecutes;
    processExecutes.setProcess(std::make_shared<Process>(process));
    processExecutes.setChild(std::make_shared<boost::process::child>(
        path.string(), process.getArgs(),
        boost::process::std_out > boost::process::null,
        boost::process::std_err > boost::process::null));
    future = processExecutes.getFuture();
    m_processes.insert({process.getId(), processExecutes});
  }
  process.onStart();
  return future;
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
    // monitorProcessGroups();
  }
}

void ProcessService::monitorProcessGroups() {
  /*for (const auto& [name, processGroup] : m_processGroups) {
    auto processes = processGroup->getProcesses();
    bool remove = true;
    for (const auto& process : processes) {
      ProcessMap::accessor found;
      m_processes.find(found, process.getId());
      if (!found.empty()) {
        remove = false;
      }
    }
    if (remove) {
      m_processGroups.erase(name);
    }
  }*/
}

void ProcessService::monitorProcess() {
  std::lock_guard<std::mutex> locker(m_processesMutex);
  // check state of process
  std::vector<std::string> removes;
  for (auto& [name, processExecutes] : m_processes) {
    bool running = processExecutes.getChild()->running();
    if (running) {
      continue;
    }
    if (processExecutes.getProcess()->getMaxAutoRestarts() != 0 &&
        processExecutes.getProcess()->currentRestarts() <
            processExecutes.getProcess()->getMaxAutoRestarts()) {
      processExecutes.getProcess()->increaseRestarts();
      std::filesystem::path path;
      if (!is_regular_file(processExecutes.getProcess()->getPath())) {
        std::string tmp = processExecutes.getProcess()->getPath().string();
        auto optPath = m_environmentConfiguration->pathOf(tmp);
        if (!optPath.has_value()) {
          throw std::runtime_error("path invalid");
        }
        path = optPath.value();
      }
      processExecutes.setChild(std::make_shared<boost::process::child>(
          path.string(), processExecutes.getProcess()->getArgs(),
          boost::process::std_out > boost::process::null,
          boost::process::std_err > boost::process::null));
      LOG_INFO("{}/{} process restarted", processExecutes.getProcess()->getId(),
               processExecutes.getProcess()->getPath().filename().string());
      processExecutes.getProcess()->onRestart();
      continue;
    }
    int exitCode = processExecutes.getChild()->exit_code();
    processExecutes.setPromiseValue(exitCode);
    processExecutes.getProcess()->onStop();
    removes.push_back(name);
  }
  for (const auto& name : removes) {
    m_processes.erase(name);
  }
}

ProcessService::~ProcessService() {
  if (m_monitorThread.joinable()) {
    m_monitorThread.join();
  }
}
bool ProcessService::stopOf(const Process& process) {
  ProcessExecutes processExecutes;
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    auto found = m_processes.find(process.getId());
    if (found == m_processes.end()) {
      return true;
    }
    processExecutes = found->second;
  }
  LOG_INFO("{}/{} send kill {} SIGINT", process.getId(),
           process.getPath().filename().string(),
           processExecutes.getChild()->id());
  SignalService::kill(processExecutes.getChild()->id(), SIGINT);
  bool success = true;
  // only wait if process is still running
  if (processExecutes.getChild()->running()) {
    LOG_INFO("{}/{} -> process is still running with  {}", process.getId(),
             process.getPath().filename().string(),
             processExecutes.getChild()->id());
    std::condition_variable cond;
    std::mutex mutex;
    std::unique_lock lock(mutex);
    bool ret = cond.wait_for(
        lock, m_processStopWaitTime->getValue(),
        [&]() -> bool { return !processExecutes.getChild()->running(); });
    LOG_INFO("{}/{} -> process shutdown {}", process.getId(),
             process.getPath().filename().string(), ret ? "true" : "false");
  }
  if (!processExecutes.getChild()->running()) {
    processExecutes.setPromiseValue(processExecutes.getChild()->exit_code());
    processExecutes.getProcess()->onStop();
  }
  if (success) {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    m_processes.erase(processExecutes.getProcess()->getId());
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
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    auto found = m_processes.find(process.getId());
    if (found == m_processes.end()) {
      return;
    }
    found->second.getProcess()->onRestart();
  }
}
void ProcessService::terminateOf(const Process& process) {
  ProcessExecutes processExecutes;
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    auto found = m_processes.find(process.getId());
    if (found == m_processes.end()) {
      return;
    }
    processExecutes = found->second;
  }
  bool running = processExecutes.getChild()->running();
  if (running) {
    processExecutes.getChild()->terminate();
  }
  processExecutes.setPromiseValue(processExecutes.getChild()->exit_code());
  processExecutes.getProcess()->onTerminate();
  m_processes.erase(processExecutes.getProcess()->getId());
}
void ProcessService::detachOf(const Process& process) {
  ProcessExecutes processExecutes;
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    auto found = m_processes.find(process.getId());
    if (found == m_processes.end()) {
      LOG_ERROR("{}/{} process not found", process.getId(),
                process.getPath().filename().string());
      return;
    }
    processExecutes = found->second;
  }

  // detach process => erase process from list bc. of separate running process
  // with no further monitoring
  processExecutes.getChild()->detach();
  // inform about success exit bc. no further monitoring
  processExecutes.setPromiseValue(EXIT_SUCCESS);
  m_processes.erase(processExecutes.getProcess()->getId());
}
void ProcessService::startOf(const ProcessGroup& processGroup) {
  {
    std::lock_guard<std::mutex> locker(m_processGroupMutex);
    auto found = m_processGroups.find(processGroup.getId());
    if (found != m_processGroups.end()) {
      LOG_ERROR("{}/{} processGroup exits", processGroup.getId(),
                processGroup.getName());
      return;
    }
  }
  {
    std::lock_guard<std::mutex> locker(m_processGroupMutex);
    m_processGroups.insert(
        {processGroup.getId(), std::make_shared<ProcessGroup>(processGroup)});
  }
  for (const auto& process : processGroup.getProcesses()) {
    startOf(process);
  }
}
bool ProcessService::stopOf(const ProcessGroup& processGroup) {
  {
    std::lock_guard<std::mutex> locker(m_processGroupMutex);
    auto found = m_processGroups.find(processGroup.getId());
    if (found == m_processGroups.end()) {
      LOG_ERROR("{}/{} processGroup doesn't exists => stop not available",
                processGroup.getId(), processGroup.getName());
      return true;
    }
  }
  bool success = true;
  for (const auto& process : processGroup.getProcesses()) {
    bool ret = stopOf(process);
    if (!ret) {
      success = false;
    }
  }
  if (success) {
    std::lock_guard<std::mutex> locker(m_processGroupMutex);
    m_processGroups.erase(processGroup.getId());
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
  {
    std::lock_guard<std::mutex> locker(m_processGroupMutex);
    auto found = m_processGroups.find(processGroup.getId());
    if (found == m_processGroups.end()) {
      LOG_ERROR("{}/{} processGroup doesn't exists => no terminate",
                processGroup.getId(), processGroup.getName());
      return;
    }
  }
  for (const auto& process : processGroup.getProcesses()) {
    terminateOf(process);
  }
  m_processGroups.erase(processGroup.getId());
}
void ProcessService::detachOf(const ProcessGroup& processGroup) {
  {
    std::lock_guard<std::mutex> locker(m_processGroupMutex);
    auto found = m_processGroups.find(processGroup.getId());
    if (found == m_processGroups.end()) {
      LOG_ERROR("{}/{} processGrouop doesn't exists => no detach available");
      return;
    }
  }
  for (const auto& process : processGroup.getProcesses()) {
    detachOf(process);
  }
  m_processGroups.erase(processGroup.getId());
}
ProcessInfo ProcessService::currentOf() {
  ProcessInfo info(m_process, boost::this_process::get_id(), true, -1, "", "");
  return info;
}
