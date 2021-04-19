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
    : enabled(enabled),
      automaticRestart(automaticRestart),
      maxRestarts(maxRestarts),
      startSequence(startSequence),
      name(std::move(name)),
      path(std::move(path)),
      args(std::move(args)) {}

int64_t Process::getId() const { return id; }

void Process::setId(int64_t id) { Process::id = id; }

bool Process::isEnabled() const { return enabled; }

void Process::setEnable(bool enable) { enabled = enable; }

bool Process::isAutomaticRestart() const { return automaticRestart; }

int Process::getRestarts() const { return restarts; }

void Process::setRestarts(int restarts) { Process::restarts = restarts; }

int Process::getMaxRestarts() const { return maxRestarts; }

int Process::getStartSequence() const { return startSequence; }

const std::string &Process::getName() const { return name; }

const std::filesystem::path &Process::getPath() const { return path; }

const std::vector<std::string> &Process::getArgs() const { return args; }

const std::shared_ptr<boost::process::child> &Process::getChild() const {
  return child;
}

int Process::getExitCode() const { return exitCode; }

void Process::setExitCode(int exitCode) { Process::exitCode = exitCode; }
void Process::startChild() {
  exitCode = -1;
  std::filesystem::path newPath(
      path.string() + std::filesystem::path::preferred_separator + name);
  child = std::make_shared<boost::process::child>(
      newPath.string(), args, boost::process::std_out > boost::process::null,
      boost::process::std_err > stderr);
}
