#include "base_library/features/base/models/Process.h"
bool Process::ProcessComparator::operator()(const Process &left,
                                            const Process &right) const {
  return right.getStartSequence() > left.getStartSequence();
}

std::ostream &operator<<(std::ostream &os, const Process::ProcessInfo &info) {
  os << "id: " << info.id << " enabled: " << info.enabled
     << " name: " << info.name << " running: " << info.running
     << " processId: " << info.processId << " exitCode: " << info.exitCode;
  return os;
}

Process::Process(int startSequence, std::string name,
                 std::filesystem::path path, std::vector<std::string> args,
                 bool enabled, bool automaticRestart, int maxRestarts)
    : m_enabled(enabled),
      m_automaticRestart(automaticRestart),
      m_maxRestarts(maxRestarts),
      m_startSequence(startSequence),
      m_name(std::move(name)),
      m_path(std::move(path)),
      m_args(std::move(args)) {}

int64_t Process::getId() const { return m_id; }

void Process::setId(int64_t newId) { Process::m_id = newId; }

bool Process::isEnabled() const { return m_enabled; }

void Process::setEnable(bool enable) { m_enabled = enable; }

bool Process::isAutomaticRestart() const { return m_automaticRestart; }

int Process::getRestarts() const { return m_restarts; }

void Process::setRestarts(int newRestarts) {
  Process::m_restarts = newRestarts;
}

int Process::getMaxRestarts() const { return m_maxRestarts; }

int Process::getStartSequence() const { return m_startSequence; }

const std::string &Process::getName() const { return m_name; }

const std::filesystem::path &Process::getPath() const { return m_path; }

const std::vector<std::string> &Process::getArgs() const { return m_args; }

const std::shared_ptr<boost::process::child> &Process::getChild() const {
  return m_child;
}

int Process::getExitCode() const { return m_exitCode; }

void Process::setExitCode(int newExitCode) {
  Process::m_exitCode = newExitCode;
}
void Process::startChild() {
  m_exitCode = -1;
  std::filesystem::path newPath(
      m_path.string() + std::filesystem::path::preferred_separator + m_name);
  m_child = std::make_shared<boost::process::child>(
      newPath.string(), m_args, boost::process::std_out > boost::process::null,
      boost::process::std_err > stderr);
}
