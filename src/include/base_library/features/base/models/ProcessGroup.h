#ifndef CPP_BASE_LIBRARY_PROCESSGROUP_H
#define CPP_BASE_LIBRARY_PROCESSGROUP_H

#include <vector>

#include "base_library/features/base/models/Process.h"

class ProcessGroup {
 private:
  std::string m_id = UUID::generate();
  std::string m_name;
  std::vector<Process> m_processes;

 public:
  ProcessGroup(std::string name, std::vector<Process> processes);
  [[nodiscard]] const std::string& getId() const;
  [[nodiscard]] const std::vector<Process>& getProcesses() const;
  [[nodiscard]] const std::string& getName() const;
};

#endif  // CPP_BASE_LIBRARY_PROCESSGROUP_H
