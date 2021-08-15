#include "base_library/core/StartupBuilder.h"

#include <algorithm>
#include <csignal>

#include "base_library/core/services/BootstrapService.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/core/services/PersistableService.h"
#include "base_library/core/services/ProcessService.h"
#include "base_library/features/base/models/ProcessName.h"
#include "base_library/features/base/services/InitializeService.h"

StartupBuilder *StartupBuilder::m_startupBuilder = nullptr;

StartupBuilder::StartupBuilder(Process::ProcessInfo &&processInfo,
                               ProcessName &&name)
    : m_processInfo(
          std::make_shared<Process::ProcessInfo>(std::move(processInfo))),
      m_name(std::make_shared<ProcessName>(std::move(name))) {
  signal(SIGINT, StartupBuilder::receiveSignal);
  signal(SIGCHLD, StartupBuilder::receiveSignal);
  signal(SIGTERM, StartupBuilder::receiveSignal);
}

StartupBuilder &StartupBuilder::with(int argc, char *argv[]) {
  Process::ProcessInfo info = ProcessService::ofCurrentProcess(argc, argv);
  ProcessName name(argc, argv);
  DECLARE_LOGGER(info.name);
  m_startupBuilder = new StartupBuilder(std::move(info), std::move(name));
  return *m_startupBuilder;
}

void StartupBuilder::withOutFeature(std::string_view nameOfFeature) {
  m_features.erase(std::remove_if(m_features.begin(), m_features.end(),
                                  [&nameOfFeature](auto &&feature) -> bool {
                                    return feature->getName() == nameOfFeature;
                                  }),
                   m_features.end());
}

StartupBuilder &StartupBuilder::start() {
  Hypodermic::ContainerBuilder builder;
  for (auto &&feature : m_features) {
    feature->registerTypes(builder);
  }
  builder.registerInstance(m_processInfo);
  builder.registerInstance(m_name);
  m_container = builder.build();
  for (auto &&feature : m_features) {
    feature->initialize(m_container);
  }
  std::shared_ptr<BootstrapService> bootstrapService =
      m_container->resolve<BootstrapService>();
  bootstrapService->onStart();
  std::shared_ptr<PersistableService> persistableService =
      m_container->resolve<PersistableService>();
  persistableService->awake();
  std::shared_ptr<InitializeService> initializeService =
      m_container->resolve<InitializeService>();
  LOG_INFO("{} finished initialization", m_processInfo->name);
  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  m_conditionVariable.wait(lock);
  return *this;
}

void StartupBuilder::receiveSignal(int signal) {
  switch (signal) {
    case SIGINT:
    case SIGCHLD:
    case SIGTERM:
      m_startupBuilder->onShutdown();
      break;
    default:
      LOG_ERROR("{} undefined signal", signal);
      break;
  }
}

void StartupBuilder::onShutdown() {
  LOG_INFO("{} shutdown", m_processInfo->name);

  m_conditionVariable.notify_all();
  // trigger shutdown
  for (auto &&abstractService : m_abstractServices) {
    abstractService->onShutdown();
  }
}