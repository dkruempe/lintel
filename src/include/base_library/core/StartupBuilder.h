#ifndef CPP_BASE_LIBRARY_STARTUPBUILDER_H
#define CPP_BASE_LIBRARY_STARTUPBUILDER_H

#include <memory>
#include <vector>

#include "base_library/core/services/AbstractService.h"
#include "base_library/features/Feature.h"
#include "base_library/features/base/models/Process.h"
#include "base_library/features/base/models/ProcessName.h"

class StartupBuilder {
 private:
  std::vector<std::shared_ptr<Feature>> m_features;
  std::shared_ptr<Hypodermic::Container> m_container = nullptr;
  std::shared_ptr<Process::ProcessInfo> m_processInfo;
  std::shared_ptr<ProcessName> m_name;
  std::vector<std::shared_ptr<AbstractServiceInterface>> m_abstractServices;
  static StartupBuilder *m_startupBuilder;
  std::condition_variable m_conditionVariable;

  explicit StartupBuilder(Process::ProcessInfo &&processInfo,
                          ProcessName &&processName);

 public:
  StartupBuilder() = delete;

  static StartupBuilder &with(int argc, char *argv[]);

  template <typename FEATURE>
  StartupBuilder &addFeature() {
    m_features.push_back(std::make_shared<FEATURE>());
    return *this;
  }

  void withOutFeature(std::string_view nameOfFeature);

  void onShutdown();

  static void receiveSignal(int signal);

  StartupBuilder &start();
};

#endif  // CPP_BASE_LIBRARY_STARTUPBUILDER_H
