#include "base_library/core/services/ProcessService.h"

#include <base_library/features/base/configuration/ProcessEntry.h>

#include <boost/process/v1/child.hpp>
#include <boost/process/v1/env.hpp>
#include <boost/process/v1/environment.hpp>
#include <boost/process/v1/io.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <regex>
#include <stdexcept>
#include <thread>

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
  std::shared_ptr<IHistoryService> historyService)
  : PropertyRegistration<ProcessService>(processName->getProcessName()), m_processName(std::move(processName)),
    m_environmentConfiguration(std::move(environmentConfiguration)), m_configuration(std::move(configuration)),
    m_historyService(std::move(historyService))
{
  m_monitorWaitTime = registerProperty<std::chrono::seconds>(
    "m_monitorWaitTime", std::chrono::seconds(1), "Wait time after monitor cycle", true, __FILE__, __LINE__);
  m_processStopWaitTime = registerProperty<std::chrono::milliseconds>("m_processStopWaitTime",
    std::chrono::milliseconds(200),
    "Wait time for stopping a process",
    true,
    __FILE__,
    __LINE__);
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

std::filesystem::path ProcessService::resolvePath(const std::filesystem::path &path) const
{
  if (is_regular_file(path)) { return path; }
  auto optPath = m_environmentConfiguration->pathOf(path.string());
  if (!optPath.has_value()) { throw std::runtime_error(path.filename().string() + ": is not a file"); }
  return optPath.value();
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
        temp.push_back(processInfoOf(id, process, found->second->getName(), found->second->getId()));
      } else {
        temp.push_back(processInfoOf(id, process, "none", "none"));
      }
    }
  }
  temp.push_back(currentOf());
  return temp;
}

ProcessInfo ProcessService::processInfoOf(const std::string &id,
  ProcessExecutes &processExecutes,
  const std::string &groupName,
  const std::string &groupId)
{
  bool running = processExecutes.getChild()->running();
  // exit_code() is only meaningful after the child has been waited on;
  // report invalid instead of faking a value for running/unknown children.
  bool exitCodeValid = !running;
  int exitCode = 0;
  if (exitCodeValid) {
    try {
      exitCode = processExecutes.getChild()->exit_code();
    } catch (const std::exception &) {
      exitCodeValid = false;
    }
  }
  if (running) { refreshResourceSampleIfStale(processExecutes); }
  ProcessInfo info(processExecutes.getProcess(),
    processExecutes.getChild()->id(),
    running,
    exitCode,
    groupName,
    groupId,
    exitCodeValid,
    processExecutes.resourceSample());
  static_cast<void>(id);
  return info;
}

std::shared_ptr<boost::process::v1::child> ProcessService::spawnChild(const std::filesystem::path &path,
  const std::vector<std::string> &args,
  const std::string &configName)
{
  if (configName.empty()) {
    return std::make_shared<boost::process::v1::child>(path.string(),
      args,
      boost::process::v1::std_out > boost::process::v1::null,
      boost::process::v1::std_err > boost::process::v1::null);
  }
  // the child loads its own bootstrap config (e.g. a DB-free variant)
  return std::make_shared<boost::process::v1::child>(path.string(),
    args,
    boost::process::v1::env["BOOTSTRAP_CONFIG_NAME"] = configName,
    boost::process::v1::std_out > boost::process::v1::null,
    boost::process::v1::std_err > boost::process::v1::null);
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

  // II resolve path of process
  std::filesystem::path path = resolvePath(process.getPath());

  // III insert process (spawn outside the mutex to avoid blocking other callers)
  std::shared_ptr<boost::process::v1::child> child = spawnChild(path, process.getArgs(), process.getConfigName());
  std::future<int> future;
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    ProcessExecutes processExecutes;
    processExecutes.setProcess(std::make_shared<Process>(process));
    auto found = m_processes.find(process.getId());
    if (found != m_processes.end()) {
      throw std::runtime_error(
        process.getId() + "/" + process.getPath().filename().string() + ": process already found in list");
    }
    processExecutes.setChild(child);
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
    try {
      monitorProcess();
      monitorProcessGroups();
    } catch (const std::exception &exception) {
      LOG_ERROR("monitorProcess failed: {}", exception.what());
    }
  }
}

std::chrono::milliseconds backoffDelayFor(const Process &process, int failures)
{
  const std::chrono::milliseconds base = process.getRestartDelay();
  if (base.count() <= 0 || failures <= 0) { return std::chrono::milliseconds(0); }
  std::chrono::milliseconds delay = base;
  for (int i = 1; i < failures && delay.count() < INT64_MAX / 2; i++) { delay *= 2; }
  const std::chrono::milliseconds maximum = process.getRestartDelayMax();
  if (maximum.count() > 0 && delay > maximum) { delay = maximum; }
  return delay;
}

void ProcessService::monitorProcess()
{
  // snapshot the entries that need handling so that blocking spawn/wait calls
  // happen outside the mutex (avoids stalling query/lifecycle operations).
  struct Action
  {
    std::string id;
    std::shared_ptr<Process> process;
    std::shared_ptr<boost::process::v1::child> child;
    bool running = false;
    int consecutiveFailures = 0;
    bool failed = false;
    std::chrono::steady_clock::time_point processStartTime{};
    std::chrono::steady_clock::time_point lastRestartTime{};
    std::optional<ProcessResourceData> resourceSample;
    std::chrono::steady_clock::time_point lastNotifyTime{};
    enum Kind { None, Sample, Restart, Spawn, Finalize } kind = None;
  };

  std::vector<Action> actions;
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    for (auto &[id, exec] : m_processes) {
      Action action;
      action.id = id;
      action.process = exec.getProcess();
      action.child = exec.getChild();
      action.consecutiveFailures = exec.consecutiveFailures();
      action.failed = exec.isFailed();
      action.processStartTime = exec.processStartTime();
      action.lastRestartTime = exec.lastRestartTime();
      action.resourceSample = exec.resourceSample();
      action.lastNotifyTime = exec.lastNotifyTime();
      action.running = action.child->running();
      if (action.running) {
        action.kind = Action::Sample;
      } else {
        action.kind = Action::Finalize;// default; overridden below if restartable
      }
      actions.push_back(std::move(action));
    }
  }

  std::vector<Action> toSpawn;
  std::vector<Action> toFinalize;
  for (auto &action : actions) {
    if (action.running) { continue; }
    // a process that has moved to the failed state is left in the map so that
    // group-level automation (monitorProcessGroups) or a manual restart can
    // recover it; it is not auto-restarted here to break the crash loop.
    if (action.failed) {
      action.kind = Action::None;
      continue;
    }
    bool restartable = false;
    // a stopped process may be restarted if automation allows it
    const bool unlimited = action.process->getMaxAutoRestarts() == -1;
    const bool restartsLeft = unlimited || action.process->currentRestarts() < action.process->getMaxAutoRestarts();
    if (action.process->getMaxAutoRestarts() != 0 && restartsLeft) {
      // reset the failure counter if the last run was healthy (long enough)
      const auto uptime = std::chrono::steady_clock::now() - action.processStartTime;
      if (action.process->getMinUptime().count() <= 0 || uptime >= action.process->getMinUptime()) {
        action.consecutiveFailures = 0;
      }
      // clear the failure counter when the restart window has passed
      if (action.process->getRestartWindow().count() > 0) {
        const auto sinceLast = std::chrono::steady_clock::now() - action.lastRestartTime;
        if (sinceLast > action.process->getRestartWindow()) { action.consecutiveFailures = 0; }
      }
      // circuit breaker: too many fast consecutive failures
      const int rateLimit = action.process->getMaxRestartRate();
      if (rateLimit >= 0 && action.consecutiveFailures >= rateLimit) {
        action.failed = true;
        LOG_ERROR("{}/{} moved to failed state after {} fast failures",
          action.process->getId(),
          action.process->getPath().filename().string(),
          action.consecutiveFailures);
        DEFINE_HISTORY_ENTRY(historyEntry,
          "PROCESS",
          fmt::format("{}/{} moved to failed state after {} fast failures",
            action.process->getId(),
            action.process->getPath().filename().string(),
            action.consecutiveFailures));
        m_historyService->historizeOf({ historyEntry });
        // leave the entry in the map for group-level recovery
        action.kind = Action::None;
        continue;
      }
      const auto delay = backoffDelayFor(*action.process, action.consecutiveFailures);
      const auto notBefore = action.lastRestartTime + delay;
      if (std::chrono::steady_clock::now() >= notBefore) {
        action.kind = Action::Spawn;
        restartable = true;
      }
      // otherwise wait for the backoff delay before restarting (kept in map)
    }
    if (!restartable) { toFinalize.push_back(action); }
    if (action.kind == Action::Spawn) { toSpawn.push_back(action); }
  }

  // blocking step: spawn new children and wait for finalized exits, outside the mutex
  struct SpawnResult
  {
    std::string id;
    std::shared_ptr<boost::process::v1::child> child;
  };
  std::vector<SpawnResult> spawned;
  for (auto &action : toSpawn) {
    try {
      auto child =
        spawnChild(resolvePath(action.process->getPath()), action.process->getArgs(), action.process->getConfigName());
      spawned.push_back({ action.id, child });
    } catch (const std::exception &exception) {
      LOG_ERROR("{}/{} restart failed: {}",
        action.process->getId(),
        action.process->getPath().filename().string(),
        exception.what());
    }
  }

  struct FinalizeResult
  {
    std::string id;
    int exitCode = 0;
  };
  std::vector<FinalizeResult> finalized;
  for (auto &action : toFinalize) {
    int exitCode = 0;
    try {
      action.child->wait();
      exitCode = action.child->exit_code();
    } catch (const std::exception &exception) {
      LOG_ERROR("{}/{} failed to read exit code: {}",
        action.process->getId(),
        action.process->getPath().filename().string(),
        exception.what());
    }
    finalized.push_back({ action.id, exitCode });
  }

  // apply step: mutate state under the mutex
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    for (auto &result : spawned) {
      auto found = m_processes.find(result.id);
      if (found == m_processes.end()) { continue; }
      auto &exec = found->second;
      exec.setChild(result.child);
      exec.setConsecutiveFailures(exec.consecutiveFailures() + 1);
      exec.setFailed(false);
      exec.setResourceSample(ProcessResourceData{});
      exec.getProcess()->increaseRestarts();
      exec.getProcess()->onRestart();
      DEFINE_HISTORY_ENTRY(historyEntry,
        "PROCESS",
        fmt::format(
          "{}/{} process restarted", exec.getProcess()->getId(), exec.getProcess()->getPath().filename().string()));
      m_historyService->historizeOf({ historyEntry });
      LOG_INFO("{}/{} process restarted", exec.getProcess()->getId(), exec.getProcess()->getPath().filename().string());
    }
    for (auto &result : finalized) {
      auto found = m_processes.find(result.id);
      if (found == m_processes.end()) { continue; }
      auto &exec = found->second;
      // only finalize once
      try {
        exec.setPromiseValue(result.exitCode);
      } catch (const std::exception &) {
        // promise already fulfilled
      }
      exec.getProcess()->onStop();
      DEFINE_HISTORY_ENTRY(historyEntry,
        "PROCESS",
        fmt::format(
          "{}/{} process stopped", exec.getProcess()->getId(), exec.getProcess()->getPath().filename().string()));
      m_historyService->historizeOf({ historyEntry });
      LOG_INFO("{}/{} process stopped", exec.getProcess()->getId(), exec.getProcess()->getPath().filename().string());
      m_processes.erase(result.id);
    }
  }

  // non-blocking step: sample resources and handle periodic restart assignment.
  // stopOf re-acquires the processes mutex, so periodic-restart targets are
  // collected first and processed after the lock is released.
  std::vector<std::shared_ptr<Process>> periodicRestarts;
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    for (auto &action : actions) {
      if (action.kind != Action::Sample && action.kind != Action::Finalize) { continue; }
      auto found = m_processes.find(action.id);
      if (found == m_processes.end()) { continue; }
      auto &exec = found->second;
      if (!exec.getChild()->running()) { continue; }
      sampleResources(action.id, exec);
      const auto interval = exec.getProcess()->getRestartInterval();
      if (interval.count() <= 0 || !exec.getProcess()->inActiveWindow()) { continue; }
      const auto sinceStart = std::chrono::steady_clock::now() - exec.processStartTime();
      if (sinceStart >= interval) {
        exec.markRestart();
        exec.setConsecutiveFailures(0);
        periodicRestarts.push_back(exec.getProcess());
      }
    }
  }
  for (const auto &process : periodicRestarts) { stopOf(*process); }
}

void ProcessService::refreshResourceSampleIfStale(ProcessExecutes &processExecutes)
{
  // Nothing is ever read more often than the monitor's notify cadence, so a
  // short staleness window keeps the cached sample fresh without hammering
  // the OS per read. Keeps health/CLI display current even before the monitor
  // thread's first tick (queries can race ahead of the 1s cycle).
  const auto &cached = processExecutes.resourceSample();
  const auto age = cached.has_value() ? std::chrono::steady_clock::now() - cached->sampleTime
                                      : std::chrono::steady_clock::duration::max();
  if (cached.has_value() && cached->valid && age <= std::chrono::milliseconds(500)) { return; }
  auto fresh = ProcessResourceReader::readOf(processExecutes.getChild()->id(), cached);
  processExecutes.setResourceSample(fresh);
}

void ProcessService::sampleResources(const std::string &id, ProcessExecutes &processExecutes)
{
  auto newSample = ProcessResourceReader::readOf(processExecutes.getChild()->id(), processExecutes.resourceSample());
  processExecutes.setResourceSample(newSample);
  if (!newSample.valid) { return; }

  bool reachedCpu = false, reachedMemory = false;
  if (processExecutes.getProcess()->getCpuNotify().has_value() && newSample.cpuPercent.has_value()) {
    reachedCpu = newSample.cpuPercent.value() >= processExecutes.getProcess()->getCpuNotify().value();
  }
  if (processExecutes.getProcess()->getMemoryNotify().has_value() && newSample.memoryBytes.has_value()) {
    reachedMemory = newSample.memoryBytes.value() >= processExecutes.getProcess()->getMemoryNotify().value();
  }
  if (!reachedCpu && !reachedMemory) { return; }
  // rate-limit notify to once per monitor period
  const auto now = std::chrono::steady_clock::now();
  if (processExecutes.lastNotifyTime().time_since_epoch().count() == 0
      || now - processExecutes.lastNotifyTime() >= std::chrono::seconds(1)) {
    processExecutes.setLastNotifyTime(now);
    LOG_WARN("{} resources exceeded thresholds: cpu={} mem={}", id, reachedCpu, reachedMemory);
    DEFINE_HISTORY_ENTRY(historyEntry,
      "PROCESS",
      fmt::format("{}/{} resource threshold exceeded (cpu={} mem={})",
        processExecutes.getProcess()->getId(),
        processExecutes.getProcess()->getPath().filename().string(),
        reachedCpu,
        reachedMemory));
    m_historyService->historizeOf({ historyEntry });
  }
}

void ProcessService::monitorProcessGroups()
{
  // group-level automation: recover processes of a group that are in the failed
  // state (flagged by the per-process circuit breaker). The group container is
  // left intact; the failed child is re-spawned in place outside the mutex.
  struct Recovery
  {
    std::string id;
    std::shared_ptr<Process> process;
    std::shared_ptr<boost::process::v1::child> child;
  };
  std::vector<Recovery> toRecover;
  {
    std::lock_guard<std::mutex> groupLocker(m_processGroupMutex);
    std::lock_guard<std::mutex> locker(m_processesMutex);
    for (const auto &[groupId, group] : m_processGroups) {
      static_cast<void>(groupId);
      for (const auto &process : group->getProcesses()) {
        auto found = m_processes.find(process.getId());
        if (found == m_processes.end()) { continue; }
        if (!found->second.isFailed()) { continue; }
        if (found->second.getChild()->running()) { continue; }
        toRecover.push_back(Recovery{ process.getId(), found->second.getProcess(), found->second.getChild() });
      }
    }
  }
  for (const auto &recovery : toRecover) {
    std::shared_ptr<boost::process::v1::child> child;
    try {
      child = spawnChild(
        resolvePath(recovery.process->getPath()), recovery.process->getArgs(), recovery.process->getConfigName());
    } catch (const std::exception &exception) {
      LOG_ERROR("{}/{} group recovery failed: {}",
        recovery.process->getId(),
        recovery.process->getPath().filename().string(),
        exception.what());
      continue;
    }
    {
      std::lock_guard<std::mutex> locker(m_processesMutex);
      auto found = m_processes.find(recovery.id);
      if (found == m_processes.end()) { continue; }
      found->second.setChild(child);
      found->second.setFailed(false);
      found->second.setConsecutiveFailures(0);
      found->second.setResourceSample(ProcessResourceData{});
      found->second.markRestart();
      found->second.getProcess()->increaseRestarts();
      found->second.getProcess()->onRestart();
    }
    LOG_INFO(
      "{}/{} recovered from failed state", recovery.process->getId(), recovery.process->getPath().filename().string());
    DEFINE_HISTORY_ENTRY(historyEntry,
      "PROCESS",
      fmt::format("{}/{} recovered from failed state",
        recovery.process->getId(),
        recovery.process->getPath().filename().string()));
    m_historyService->historizeOf({ historyEntry });
  }
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
  bool stopped = !processExecutes.getChild()->running();
  // only wait if process is still running
  if (!stopped) {
    LOG_INFO("{}/{} -> process is still running with  {}",
      process.getId(),
      process.getPath().filename().string(),
      processExecutes.getChild()->id());
    const auto deadline = std::chrono::steady_clock::now() + m_processStopWaitTime->getValue();
    do {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      stopped = !processExecutes.getChild()->running();
    } while (!stopped && std::chrono::steady_clock::now() < deadline);
    LOG_INFO("{}/{} -> process shutdown {}",
      process.getId(),
      process.getPath().filename().string(),
      stopped ? "true" : "false");
  }
  if (!stopped) {
    // process still running => keep entry so that terminateOf can force-kill it
    // and the promise can still be fulfilled by monitorProcess
    return false;
  }
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    // monitorProcess may have already detected the stop and fulfilled the promise
    auto found = m_processes.find(process.getId());
    if (found == m_processes.end()) { return true; }
    // process may have been auto-restarted while we waited => keep the new child
    if (found->second.getChild() != processExecutes.getChild()) { return false; }
    try {
      found->second.setPromiseValue(processExecutes.getChild()->exit_code());
    } catch (const std::exception &) {
      // promise already fulfilled
    }
    found->second.getProcess()->onStop();
    m_processes.erase(found);
  }
  DEFINE_HISTORY_ENTRY(historyEntry,
    "PROCESS",
    fmt::format("{}/{} process stopped",
      processExecutes.getProcess()->getId(),
      processExecutes.getProcess()->getPath().filename().string()));
  m_historyService->historizeOf({ historyEntry });
  return true;
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
    found->second.setConsecutiveFailures(0);
    found->second.setFailed(false);
    found->second.markRestart();
  }
  DEFINE_HISTORY_ENTRY(
    historyEntry, "PROCESS", fmt::format("{}/{} restarted", process.getId(), process.getPath().filename().string()));
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
  if (processExecutes.getChild()->running()) { processExecutes.getChild()->wait(); }
  int exitCode = processExecutes.getChild()->exit_code();
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    // monitorProcess may have already detected the stop and fulfilled the promise
    auto found = m_processes.find(process.getId());
    if (found == m_processes.end()) { return; }
    try {
      found->second.setPromiseValue(exitCode);
    } catch (const std::exception &) {
      // promise already fulfilled
    }
    m_processes.erase(found);
  }
  processExecutes.getProcess()->onTerminate();
  DEFINE_HISTORY_ENTRY(historyEntry,
    "PROCESS",
    fmt::format("{}/{} process terminated", process.getId(), process.getPath().filename().string()));
  m_historyService->historizeOf({ historyEntry });
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

  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    // monitorProcess may have already detected the stop and fulfilled the promise
    auto found = m_processes.find(process.getId());
    if (found == m_processes.end()) { return; }
    processExecutes.getChild()->detach();
    // inform about success exit bc. no further monitoring
    try {
      processExecutes.setPromiseValue(EXIT_SUCCESS);
    } catch (const std::exception &) {
      // promise already fulfilled
    }
    m_processes.erase(found);
  }
  DEFINE_HISTORY_ENTRY(historyEntry,
    "PROCESS",
    fmt::format("{}/{} process detached",
      processExecutes.getProcess()->getId(),
      processExecutes.getProcess()->getPath().filename().string()));
  m_historyService->historizeOf({ historyEntry });
}

void ProcessService::resetOf(const Process &process)
{
  if (!m_running) { return; }
  {
    std::lock_guard<std::mutex> locker(m_processesMutex);
    auto found = m_processes.find(process.getId());
    if (found == m_processes.end()) {
      LOG_WARN("{}/{} process not found for reset", process.getId(), process.getPath().filename().string());
      return;
    }
    found->second.getProcess()->resetRestarts();
    found->second.setConsecutiveFailures(0);
    found->second.setFailed(false);
    found->second.markRestart();
  }
  LOG_INFO("{}/{} reset restarts and failure state", process.getId(), process.getPath().filename().string());
  DEFINE_HISTORY_ENTRY(historyEntry,
    "PROCESS",
    fmt::format("{}/{} reset restarts and failure state", process.getId(), process.getPath().filename().string()));
  m_historyService->historizeOf({ historyEntry });
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
    fmt::format("{}/{} processGroup terminated", processGroup.getId(), processGroup.getName()));
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
      LOG_ERROR(
        "{}/{} processGroup doesn't exists => no detach available", processGroup.getId(), processGroup.getName());
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
        processInfos.push_back(processInfoOf(process.getId(), found->second, iter->getName(), iter->getId()));
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
