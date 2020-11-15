#ifndef CPP_BASE_LIBRARY_PROCESSNAME_H
#define CPP_BASE_LIBRARY_PROCESSNAME_H

#include <filesystem>

class ProcessName {
private:
  std::filesystem::path path;

public:
  ProcessName(int argc, char *argv[]) { path = argv[0]; }

  ProcessName(std::string processName) : path(processName){};

  std::string getProcessName() const { return path.filename().string(); }
};

#endif // CPP_BASE_LIBRARY_PROCESSNAME_H
