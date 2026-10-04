#include "base_library/features/base/models/Process.h"

#include <ctime>

Process::Process(std::filesystem::path path, std::vector<std::string> args, std::string configName)
  : m_path(std::move(path)), m_args(std::move(args)), m_configName(std::move(configName))
{}

void Process::onStart() const
{
  if (m_onStart == nullptr) { return; }
  (*m_onStart)(*this);
}

void Process::onStop() const
{
  if (m_onStop == nullptr) { return; }
  (*m_onStop)(*this);
}

void Process::onFinish() const
{
  if (m_onFinish == nullptr) { return; }
  (*m_onFinish)(*this);
}

void Process::onRestart() const
{
  if (m_onRestart == nullptr) { return; }
  (*m_onRestart)(*this);
}

void Process::onTerminate() const
{
  if (m_onTerminate == nullptr) { return; }
  (*m_onTerminate)(*this);
}

void Process::addOnStartEvent(std::shared_ptr<std::function<void(const Process &)>> onStart)
{
  m_onStart = std::move(onStart);
}

void Process::addOnStopEvent(std::shared_ptr<std::function<void(const Process &)>> onStop)
{
  m_onStop = std::move(onStop);
}

void Process::addOnRestartEvent(std::shared_ptr<std::function<void(const Process &)>> onRestart)
{
  m_onRestart = std::move(onRestart);
}

void Process::addOnTerminateEvent(std::shared_ptr<std::function<void(const Process &)>> onTerminate)
{
  m_onTerminate = std::move(onTerminate);
}

void Process::addOnFinishEvent(std::shared_ptr<std::function<void(const Process &)>> onFinish)
{
  m_onFinish = std::move(onFinish);
}

void Process::enableAutoStart(int maxAutoRestarts)
{
  m_autoRestart = true;
  m_maxAutoRestarts = maxAutoRestarts;
}

void Process::disableAutoStart()
{
  m_autoRestart = false;
  m_maxAutoRestarts = 0;
}

const std::string &Process::getId() const { return m_id; }

const std::filesystem::path &Process::getPath() const { return m_path; }

bool Process::isAutoRestart() const { return m_autoRestart; }

int Process::getMaxAutoRestarts() const { return m_maxAutoRestarts; }

const std::vector<std::string> &Process::getArgs() const { return m_args; }

const std::string &Process::getConfigName() const { return m_configName; }

void Process::increaseRestarts() { m_restarts++; }

void Process::resetRestarts() { m_restarts = 0; }

int Process::currentRestarts() const { return m_restarts; }

void Process::setRestartDelay(std::chrono::milliseconds delay) { m_restartDelay = delay; }

std::chrono::milliseconds Process::getRestartDelay() const { return m_restartDelay; }

void Process::setRestartDelayMax(std::chrono::milliseconds delayMax) { m_restartDelayMax = delayMax; }

std::chrono::milliseconds Process::getRestartDelayMax() const { return m_restartDelayMax; }

void Process::setMinUptime(std::chrono::milliseconds minUptime) { m_minUptime = minUptime; }

std::chrono::milliseconds Process::getMinUptime() const { return m_minUptime; }

void Process::setMaxRestartRate(int maxRestartRate) { m_maxRestartRate = maxRestartRate; }

int Process::getMaxRestartRate() const { return m_maxRestartRate; }

void Process::setRestartWindow(std::chrono::milliseconds restartWindow) { m_restartWindow = restartWindow; }

std::chrono::milliseconds Process::getRestartWindow() const { return m_restartWindow; }

void Process::setRestartInterval(std::chrono::milliseconds restartInterval) { m_restartInterval = restartInterval; }

std::chrono::milliseconds Process::getRestartInterval() const { return m_restartInterval; }

void Process::setActiveFromHour(std::optional<int32_t> activeFromHour) { m_activeFromHour = activeFromHour; }

std::optional<int32_t> Process::getActiveFromHour() const { return m_activeFromHour; }

void Process::setActiveToHour(std::optional<int32_t> activeToHour) { m_activeToHour = activeToHour; }

std::optional<int32_t> Process::getActiveToHour() const { return m_activeToHour; }

bool Process::hasActiveWindow() const { return m_activeFromHour.has_value() && m_activeToHour.has_value(); }

bool Process::inActiveWindow() const
{
  if (!hasActiveWindow()) { return true; }
  const std::time_t now = std::time(nullptr);
  std::tm local{};
  // localtime_r writes into the caller's storage, localtime shares one static buffer
  // between all threads and is therefore a data race (CodeQL cpp/potentially-dangerous-function).
  if (::localtime_r(&now, &local) == nullptr) { return true; }
  const int32_t hour = local.tm_hour;
  const int32_t from = m_activeFromHour.value();
  const int32_t to = m_activeToHour.value();
  if (from <= to) { return hour >= from && hour < to; }
  // window wraps over midnight, e.g. 22:00-06:00
  return hour >= from || hour < to;
}

void Process::setCpuNotify(std::optional<double> cpuNotify) { m_cpuNotify = cpuNotify; }

std::optional<double> Process::getCpuNotify() const { return m_cpuNotify; }

void Process::setMemoryNotify(std::optional<uint64_t> memoryNotify) { m_memoryNotify = memoryNotify; }

std::optional<uint64_t> Process::getMemoryNotify() const { return m_memoryNotify; }
