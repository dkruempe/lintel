#ifndef CPP_SYSTEM_LIBRARY_PROCESS_H
#define CPP_SYSTEM_LIBRARY_PROCESS_H
#include <boost/process.hpp>
#include <filesystem>
#include <optional>
#include <string>
class Process {
 private:
  static constexpr int INFINITE_RESTARTS = -1;  // infinite restarts;
  int64_t id = -1;
  bool enabled;
  bool automaticRestart;
  int restarts = 0;
  int maxRestarts;  // -1 infinite
  int startSequence;
  std::string name;
  std::filesystem::path path;
  std::vector<std::string> args;
  std::shared_ptr<boost::process::child> child = nullptr;
  int exitCode = -1;

 public:
  // Comprators
  struct ProcessComparator {
    bool operator()(const Process &left, const Process &right) const;
  };

  // Info Process struct
  struct ProcessInfo {
    int64_t id;
    bool enabled;
    std::string name;
    bool running;
    pid_t processId;
    int exitCode;
    friend std::ostream &operator<<(std::ostream &os, const ProcessInfo &info);
  };

  Process(int startSequence, std::string name, std::filesystem::path path,
          std::vector<std::string> args, bool enabled,
          bool automaticRestart = false, int maxRestarts = INFINITE_RESTARTS);

  [[nodiscard]] int64_t getId() const;
  void setId(int64_t newId);
  [[nodiscard]] bool isEnabled() const;
  void setEnable(bool enable);
  [[nodiscard]] bool isAutomaticRestart() const;
  [[nodiscard]] int getRestarts() const;
  void setRestarts(int newRestarts);
  [[nodiscard]] int getMaxRestarts() const;
  [[nodiscard]] int getStartSequence() const;
  [[nodiscard]] const std::string &getName() const;
  [[nodiscard]] const std::filesystem::path &getPath() const;
  [[nodiscard]] const std::vector<std::string> &getArgs() const;
  [[nodiscard]] const std::shared_ptr<boost::process::child> &getChild() const;
  void startChild();
  [[nodiscard]] int getExitCode() const;
  void setExitCode(int newExitCode);
};
#endif  // CPP_SYSTEM_LIBRARY_PROCESS_H
