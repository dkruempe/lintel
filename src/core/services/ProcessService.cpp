#include "base_library/core/services/ProcessService.h"

#include <base_library/features/base/configuration/ProcessEntry.h>

#include <boost/process/v1.hpp>
#include <boost/process/v1/environment.hpp>
#include <boost/process/v1/io.hpp>
#include <memory>
#include <mutex>
#include <regex>
#include <stdexcept>

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/services/SignalService.h"
#include "base_library/core/utils/RegexUtils.h"
#include "base_library/features/base/configuration/ProcessComponent.h"
#include "base_library/features/base/controller/ProcessGroupDto.h"
#include "base_library/features/base/models/ProcessGroup.h"
#include "base_library/features/base/models/ProcessInfo.h"

ProcessService::ProcessService(std::shared_ptr<ProcessName> processName,
  std::shared_ptr<EnvironmentConfiguration> environmentConfiguration,
  std::shared_ptr<Configuration> configuration,
  std::shared_ptr<HistoryService> historyService)
  : PropertyRegistration<ProcessService>(processName->getProcessName()), m_processName(std::move(processName)),
    m_environmentConfiguration(std::move(environmentConfiguration)), m_configuration(std::move(configuration)),
    m_historyService(std::move(historyService))
{
  m_monitorWaitTime = registerProperty<std::chrono::seconds>(
          "m_monitorWaitTime", std::chrono::seconds(1),
          "Wait time after monitor cycle", true,
          __FILE__, __LINE__);
  m_processStopWaitTime = registerProperty<std::chrono::milliseconds>(
          "m_processStopWaitTime", std::chrono::milliseconds(200),
          "Wait time for stopping a process", true,
          __FILE__, __LINE__);
  // make sure that all variables are initialized for starting the thread
  m_process = std::make_shared<Process>(m_processName->getPath(), m_processName->getArgs());
  std::vector<std::shared_ptr<Entry>> entries = m_configuration->configurationOf<ProcessComponent>();
  for (const auto &entry : entries) {
    std::shared_ptr<ProcessEntry> processEntry = std::static_pointer_cast<ProcessEntry>(entry);
    if (processEntry->getProcess() != nullptr) {
      std::shared_ptr<Process> process = processEntry->getProcess();
      startOf(*process);
      continue;
    }
    std::shared_ptr<ProcessGroup> processGroup = processEntry->getProcessGroup();
    startOf(*processGroup);
  }
}

void ProcessService::onInitialize()
{
  m_monitorThread = std::thread([&] { run(); });
}

std::vector<ProcessInfo> ProcessService::allActiveOf()
{
  std::map<std::string, std::shared_ptr<ProcessGroup>> processGroupMap;
  {
    std::lock_guard<std::mutex> locker(m_processGroupMutex);
    for (auto &[id, processGroup] : m_processGroups) {
      for (const auto &process : processGroup->getProcesses()) {
        processGroupMap.insert({ process.getId(), processGroup });
      }
    }
  }
  std::vector<ProcessInfo> temp;
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    for (auto &[id, process] : m_processes) {
      auto found = processGroupMap.find(id);
      if (found != processGroupMap.end()) {
        temp.emplace_back(process.getProcess(),
          process.getChild()->id(),
          process.getChild()->running(),
          process.getChild()->exit_code(),
          found->second->getName(),
          found->second->getId());
      } else {
        temp.emplace_back(process.getProcess(),
          process.getChild()->id(),
          process.getChild()->running(),
          process.getChild()->exit_code(),
          "none",
          "none");
      }
    }
  }
  temp.push_back(currentOf());
  return temp;
}

std::optional<std::future<int>> ProcessService::startOf(const Process &process)
{
  if (!m_running) { return std::nullopt; }

  // I check if process is available
  {
    std::lock_guard<std::mutex> lock(m_processesMutex);
    auto found = m_processes.find(process.getId());
    if (found != m_processes.end()) {
      throw std::runtime_error(
        process.getId() + "/" + process.getPath().filename().string() + ": process already found in list");
    }
  }

  // II check path of process
  std::filesystem::path path = process.getPath();
  if (!is_regular_file(process.getPath())) {
    std::string tmp = process.getPath().string();
    auto optPath = m_environmentConfiguration->pathOf(tmp);
    if (!optPath.has_value()) {
      throw std::runtime_error(process.getId() + "/" + process.getPath().filename().string() + ": is not a file");
    }
    path = optPath.value();
  }

  // III insert process
  std::future<int> future;
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    ProcessExecutes processExecutes;
    processExecutes.setProcess(std::make_shared<Process>(process));
    processExecutes.setChild(std::make_shared<boost::process::v1::child>(path.string(),
      process.getArgs(),
      boost::process::v1::std_out > boost::process::v1::null,
      boost::process::v1::std_err > boost::process::v1::null));
    future = processExecutes.getFuture();
    m_processes.insert({ process.getId(), processExecutes });
  }
  LOG_INFO("{}/{} started", process.getId(), process.getPath().filename().string());
  DEFINE_HISTORY_ENTRY(
    historyEntry, "PROCESS", fmt::format("{}/{} started", process.getId(), process.getPath().filename().string()));
  m_historyService->historizeOf({ historyEntry });
  process.onStart();
  return std::make_optional<std::future<int>>(std::move(future));
}

void ProcessService::run()
{
  while (m_running) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait_for(lock, m_monitorWaitTime->getValue());
    monitorProcess();
    // monitorProcessGroups();
  }
}

void ProcessService::monitorProcess()
{
  std::lock_guard<std::mutex> locker(m_processesMutex);
  // check state of process
  std::vector<std::string> removes;
  for (auto &[name, processExecutes] : m_processes) {
    bool running = processExecutes.getChild()->running();
    if (running) { continue; }
    if (processExecutes.getProcess()->getMaxAutoRestarts() != 0
        && ((processExecutes.getProcess()->currentRestarts() < processExecutes.getProcess()->getMaxAutoRestarts())
            || (processExecutes.getProcess()->isAutoRestart()
                && processExecutes.getProcess()->getMaxAutoRestarts() == -1))) {
      processExecutes.getProcess()->increaseRestarts();
      std::filesystem::path path = processExecutes.getProcess()->getPath();
      if (!is_regular_file(path)) {
        std::string tmp = processExecutes.getProcess()->getPath().string();
        auto optPath = m_environmentConfiguration->pathOf(tmp);
        if (!optPath.has_value()) { throw std::runtime_error("path invalid"); }
        path = optPath.value();
      }
      processExecutes.setChild(std::make_shared<boost::process::v1::child>(path.string(),
        processExecutes.getProcess()->getArgs(),
        boost::process::v1::std_out > boost::process::v1::null,
        boost::process::v1::std_err > boost::process::v1::null));
      DEFINE_HISTORY_ENTRY(historyEntry,
        "PROCESS",
        fmt::format("{}/{} process restarted",
          processExecutes.getProcess()->getId(),
          processExecutes.getProcess()->getPath().filename().string()));
      m_historyService->historizeOf({ historyEntry });
      LOG_INFO("{}/{} process restarted",
        processExecutes.getProcess()->getId(),
        processExecutes.getProcess()->getPath().filename().string());
      processExecutes.getProcess()->onRestart();
      continue;
    }
    int exitCode = processExecutes.getChild()->exit_code();
    processExecutes.setPromiseValue(exitCode);
    processExecutes.getProcess()->onStop();
    DEFINE_HISTORY_ENTRY(historyEntry,
      "PROCESS",
      fmt::format("{}/{} process stopped",
        processExecutes.getProcess()->getId(),
        processExecutes.getProcess()->getPath().filename().string()));
    m_historyService->historizeOf({ historyEntry });
    LOG_INFO("{}/{} process stopped",
      processExecutes.getProcess()->getId(),
      processExecutes.getProcess()->getPath().filename().string());
    removes.push_back(name);
  }
  for (const auto &name : removes) { m_processes.erase(name); }
}

ProcessService::~ProcessService()
{
  try {
    m_running.store(false);
    m_condition.notify_all();
    if (m_monitorThread.joinable()) { m_monitorThread.join(); }
    std::vector<ProcessExecutes> runningProcesses;
    {
      std::lock_guard<std::mutex> locker(m_processesMutex);
      for (auto &[id, process] : m_processes) {
        if (!process.getChild()->running()) { continue; }
        runningProcesses.push_back(process);
      }
    }
    for (auto &item : runningProcesses) {
      if (!item.getChild()->running()) { continue; }
      stopOf(*item.getProcess());
    }
    for (auto &item : runningProcesses) {
      if (!item.getChild()->running()) { continue; }
      terminateOf(*item.getProcess());
    }
  } catch (const std::exception &e) {
    static_cast<void>(e);
  }
}

bool ProcessService::stopOf(const Process &process)
{
  ProcessExecutes processExecutes;
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    auto found = m_processes.find(process.getId());
    if (found == m_processes.end()) { return true; }
    processExecutes = found->second;
  }
  LOG_INFO("{}/{} send kill {} SIGINT",
    process.getId(),
    process.getPath().filename().string(),
    processExecutes.getChild()->id());
  SignalService::kill(processExecutes.getChild()->id(), SIGINT);
  bool success = true;
  // only wait if process is still running
  if (processExecutes.getChild()->running()) {
    LOG_INFO("{}/{} -> process is still running with  {}",
      process.getId(),
      process.getPath().filename().string(),
      processExecutes.getChild()->id());
    std::condition_variable cond;
    std::unique_lock lock(m_conditionMutex);
    bool ret = cond.wait_for(
      lock, m_processStopWaitTime->getValue(), [&]() -> bool { return !processExecutes.getChild()->running(); });
    LOG_INFO(
      "{}/{} -> process shutdown {}", process.getId(), process.getPath().filename().string(), ret ? "true" : "false");
  }
  if (!processExecutes.getChild()->running()) {
    processExecutes.setPromiseValue(processExecutes.getChild()->exit_code());
    processExecutes.getProcess()->onStop();
  }
  if (success) {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    m_processes.erase(processExecutes.getProcess()->getId());
  }
  DEFINE_HISTORY_ENTRY(historyEntry,
    "PROCESS",
    fmt::format("{}/{} process stopped",
      processExecutes.getProcess()->getId(),
      processExecutes.getProcess()->getPath().filename().string()));
  m_historyService->historizeOf({ historyEntry });
  return success;
}

void ProcessService::restartOf(const Process &process)
{
  if (!m_running) { return; }
  bool success = stopOf(process);
  if (!success) {
    LOG_INFO("{}/{} force stop of process", process.getId(), process.getPath().filename().string());
    terminateOf(process);
  }
  startOf(process);
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    auto found = m_processes.find(process.getId());
    if (found == m_processes.end()) { return; }
    found->second.getProcess()->onRestart();
  }
  DEFINE_HISTORY_ENTRY(historyEntry,
    "PROCESS",
    fmt::format("PROCESS", "{}/{} restarted", process.getId(), process.getPath().filename().string()));
  m_historyService->historizeOf({ historyEntry });
}

void ProcessService::terminateOf(const Process &process)
{
  ProcessExecutes processExecutes;
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    auto found = m_processes.find(process.getId());
    if (found == m_processes.end()) { return; }
    processExecutes = found->second;
  }
  bool running = processExecutes.getChild()->running();
  if (running) { processExecutes.getChild()->terminate(); }
  processExecutes.setPromiseValue(processExecutes.getChild()->exit_code());
  processExecutes.getProcess()->onTerminate();
  DEFINE_HISTORY_ENTRY(historyEntry,
    "PROCESS",
    fmt::format("{}/{} process terminated", process.getId(), process.getPath().filename().string()));
  m_historyService->historizeOf({ historyEntry });
  m_processes.erase(processExecutes.getProcess()->getId());
}

void ProcessService::detachOf(const Process &process)
{
  if (!m_running) { return; }
  ProcessExecutes processExecutes;
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    auto found = m_processes.find(process.getId());
    if (found == m_processes.end()) {
      LOG_ERROR("{}/{} process not found", process.getId(), process.getPath().filename().string());
      return;
    }
    processExecutes = found->second;
  }

  // detach process => erase process from list bc. of separate running process
  // with no further monitoring
  processExecutes.getChild()->detach();
  DEFINE_HISTORY_ENTRY(historyEntry,
    "PROCESS",
    fmt::format("{}/{} process detached",
      processExecutes.getProcess()->getId(),
      processExecutes.getProcess()->getPath().filename().string()));
  m_historyService->historizeOf({ historyEntry });
  // inform about success exit bc. no further monitoring
  processExecutes.setPromiseValue(EXIT_SUCCESS);
  m_processes.erase(processExecutes.getProcess()->getId());
}

void ProcessService::startOf(const ProcessGroup &processGroup)
{
  if (!m_running) { throw std::runtime_error("ProcessService is onShutdown => no more startOf"); }
  {
    std::lock_guard<std::mutex> locker(m_processGroupMutex);
    auto found = m_processGroups.find(processGroup.getId());
    if (found != m_processGroups.end()) {
      LOG_ERROR("{}/{} processGroup exits", processGroup.getId(), processGroup.getName());
      return;
    }
  }
  {
    std::lock_guard<std::mutex> locker(m_processGroupMutex);
    m_processGroups.insert({ processGroup.getId(), std::make_shared<ProcessGroup>(processGroup) });
  }
  for (const auto &process : processGroup.getProcesses()) { startOf(process); }
  DEFINE_HISTORY_ENTRY(
    historyEntry, "PROCESS", fmt::format("{}/{} processGroup started", processGroup.getId(), processGroup.getName()));
  m_historyService->historizeOf({ historyEntry });
}

bool ProcessService::stopOf(const ProcessGroup &processGroup)
{
  {
    std::lock_guard<std::mutex> locker(m_processGroupMutex);
    auto found = m_processGroups.find(processGroup.getId());
    if (found == m_processGroups.end()) {
      LOG_ERROR(
        "{}/{} processGroup doesn't exists => stop not available", processGroup.getId(), processGroup.getName());
      return true;
    }
  }
  bool success = true;
  for (const auto &process : processGroup.getProcesses()) {
    bool ret = stopOf(process);
    if (!ret) { success = false; }
  }
  if (success) {
    std::lock_guard<std::mutex> locker(m_processGroupMutex);
    m_processGroups.erase(processGroup.getId());
  }
  DEFINE_HISTORY_ENTRY(
    historyEntry, "PROCESS", fmt::format("{}/{} processGroup stopped", processGroup.getId(), processGroup.getName()));
  m_historyService->historizeOf({ historyEntry });
  return success;
}

void ProcessService::restartOf(const ProcessGroup &processGroup)
{
  if (!m_running) { return; }
  bool success = stopOf(processGroup);
  if (!success) { terminateOf(processGroup); }
  startOf(processGroup);
  DEFINE_HISTORY_ENTRY(
    historyEntry, "PROCESS", fmt::format("{}/{} restarted", processGroup.getId(), processGroup.getName()));
  m_historyService->historizeOf({ historyEntry });
}

void ProcessService::terminateOf(const ProcessGroup &processGroup)
{
  {
    std::lock_guard<std::mutex> locker(m_processGroupMutex);
    auto found = m_processGroups.find(processGroup.getId());
    if (found == m_processGroups.end()) {
      LOG_ERROR("{}/{} processGroup doesn't exists => no terminate", processGroup.getId(), processGroup.getName());
      return;
    }
  }
  for (const auto &process : processGroup.getProcesses()) { terminateOf(process); }
  DEFINE_HISTORY_ENTRY(historyEntry,
    "PROCESS",
    fmt::format("PROCESS", "{}/{} processGroup terminated", processGroup.getId(), processGroup.getName()));
  m_historyService->historizeOf({ historyEntry });
  m_processGroups.erase(processGroup.getId());
}
void ProcessService::detachOf(const ProcessGroup &processGroup)
{
  if (!m_running) { return; }
  {
    std::lock_guard<std::mutex> locker(m_processGroupMutex);
    auto found = m_processGroups.find(processGroup.getId());
    if (found == m_processGroups.end()) {
      LOG_ERROR("{}/{} processGrouop doesn't exists => no detach available");
      return;
    }
  }
  for (const auto &process : processGroup.getProcesses()) { detachOf(process); }
  m_processGroups.erase(processGroup.getId());
  DEFINE_HISTORY_ENTRY(
    historyEntry, "PROCESS", fmt::format("{}/{} detached", processGroup.getId(), processGroup.getName()));
  m_historyService->historizeOf({ historyEntry });
}
ProcessInfo ProcessService::currentOf()
{
  ProcessInfo info(m_process, boost::this_process::get_id(), true, -1, "", "");
  return info;
}
std::vector<ProcessGroupDto> ProcessService::allGroupsOf(const std::string &name)
{
  std::vector<std::shared_ptr<ProcessGroup>> groups;
  std::string errorMessage;
  if (!RegexUtils::validatePattern(name, errorMessage)) {
    LOG_WARN("allGroupsOf: {}", errorMessage);
    return {};
  }
  const std::regex nameRegex(name);
  {
    std::lock_guard<std::mutex> locker(m_processGroupMutex);
    for (const auto &[id, group] : m_processGroups) {
      if (!std::regex_match(group->getName(), nameRegex)) { continue; }
      groups.push_back(group);
    }
  }
  std::vector<ProcessGroupDto> dtos{};
  for (const auto &iter : groups) {
    std::vector<ProcessInfo> processInfos;
    for (const auto &process : iter->getProcesses()) {
      {
        std::lock_guard<std::mutex> locker(m_processesMutex);
        auto found = m_processes.find(process.getId());
        if (found == m_processes.end()) { continue; }
        std::shared_ptr<Process> temp = found->second.getProcess();
        boost::process::v1::pid_t id = found->second.getChild()->id();
        bool running = found->second.getChild()->running();
        int exitCode = found->second.getChild()->exit_code();
        ProcessInfo processInfo(temp, id, running, exitCode, iter->getName(), iter->getId());
        processInfos.push_back(processInfo);
      }
    }
    ProcessGroupDto dto(iter, processInfos);
    dtos.push_back(dto);
  }
  return dtos;
}
bool ProcessService::isLastProcess()
{
  std::lock_guard<std::mutex> locker(m_processesMutex);
  return m_processes.size() == 1;
}
std::optional<std::shared_ptr<Process>> ProcessService::of(const std::string &id)
{
  std::lock_guard<std::mutex> locker(m_processesMutex);
  auto found = m_processes.find(id);
  if (found == m_processes.end()) { return std::nullopt; }
  return std::make_optional(found->second.getProcess());
}