#ifndef CPP_BASE_LIBRARY_PROCESSNAME_H
#define CPP_BASE_LIBRARY_PROCESSNAME_H

#include <filesystem>

class ProcessName {
 private:
  std::filesystem::path m_path;

 public:
  ProcessName(int argc, char *argv[]) { m_path = argv[0]; }

  explicit ProcessName(std::string processName) : m_path(processName){};

  [[nodiscard]] std::string getProcessName() const {
    return m_path.filename().string();
  }
};

#endif  // CPP_BASE_LIBRARY_PROCESSNAME_H
