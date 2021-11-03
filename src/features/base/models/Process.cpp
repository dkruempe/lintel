#include "base_library/features/base/models/Process.h"

Process::Process(std::filesystem::path path, std::vector<std::string> args)
    : m_path(std::move(path)), m_args(std::move(args)) {}

void Process::onStart() {
  if (m_onStart == nullptr) {
    return;
  }
  (*m_onStart)(*this);
}
void Process::onStop() {
  if (m_onStop == nullptr) {
    return;
  }
  (*m_onStop)(*this);
}
void Process::onFinish() {
  if (m_onFinish == nullptr) {
    return;
  }
  (*m_onFinish)(*this);
}
void Process::onRestart() {
  if (m_onRestart == nullptr) {
    return;
  }
  (*m_onRestart)(*this);
}
void Process::onTerminate() {
  if (m_onTerminate == nullptr) {
    return;
  }
  (*m_onTerminate)(*this);
}
void Process::addOnStartEvent(
    std::shared_ptr<std::function<void(const Process &)>> onStart) {
  m_onStart = std::move(onStart);
}
void Process::addOnStopEvent(
    std::shared_ptr<std::function<void(const Process &)>> onStop) {
  m_onStop = std::move(onStop);
}
void Process::addOnRestartEvent(
    std::shared_ptr<std::function<void(const Process &)>> onRestart) {
  m_onRestart = std::move(onRestart);
}
void Process::addOnTerminateEvent(
    std::shared_ptr<std::function<void(const Process &)>> onTerminate) {
  m_onTerminate = std::move(onTerminate);
}
void Process::addOnFinishEvent(
    std::shared_ptr<std::function<void(const Process &)>> onFinish) {
  m_onFinish = std::move(onFinish);
}
void Process::enableAutoStart(int maxAutoRestarts) {
  m_autoRestart = true;
  m_maxAutoRestarts = maxAutoRestarts;
}
void Process::disableAutoStart() {
  m_autoRestart = true;
  m_maxAutoRestarts = -1;
}
const std::string &Process::getId() const { return m_id; }
const std::filesystem::path &Process::getPath() const { return m_path; }
bool Process::isAutoRestart() const { return m_autoRestart; }
int Process::getMaxAutoRestarts() const { return m_maxAutoRestarts; }
const std::vector<std::string> &Process::getArgs() const { return m_args; }
void Process::increaseRestarts() { m_restarts++; }
int Process::currentRestarts() const { return m_restarts; }
