#include "base_library/core/StartupBuilder.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/features/base/models/ProcessName.h"
#include "base_library/features/base/services/InitializeService.h"
#include "base_library/features/base/services/ProcessService.h"
#include <algorithm>
#include <csignal>

StartupBuilder *StartupBuilder::startupBuilder = nullptr;

StartupBuilder::StartupBuilder(Process::ProcessInfo &&processInfo,
                               ProcessName &&name)
    : processInfo(
          std::make_shared<Process::ProcessInfo>(std::move(processInfo))),
      name(std::make_shared<ProcessName>(std::move(name))) {
  signal(SIGINT, StartupBuilder::receiveSignal);
  signal(SIGCHLD, StartupBuilder::receiveSignal);
  signal(SIGTERM, StartupBuilder::receiveSignal);
}

StartupBuilder &StartupBuilder::with(int argc, char *argv[]) {
  Process::ProcessInfo info = ProcessService::ofCurrentProcess(argc, argv);
  ProcessName name(argc, argv);
  DECLARE_LOGGER(info.name);
  startupBuilder = new StartupBuilder(std::move(info), std::move(name));
  return *startupBuilder;
}

void StartupBuilder::withOutFeature(std::string_view nameOfFeature) {
  features.erase(std::remove_if(features.begin(), features.end(),
                                [&nameOfFeature](auto &&feature) -> bool {
                                  return feature->getName() == nameOfFeature;
                                }),
                 features.end());
}

StartupBuilder &StartupBuilder::start() {
  Hypodermic::ContainerBuilder builder;
  for (auto &&feature : features) {
    feature->registerTypes(builder);
  }
  builder.registerInstance(processInfo);
  builder.registerInstance(name);
  container = builder.build();
  for (auto &&feature : features) {
    feature->initialize(container);
  }
  std::shared_ptr<InitializeService> initializeService =
      container->resolve<InitializeService>();
  LOG_INFO("{} finished initialization", processInfo->name);
  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  conditionVariable.wait(lock);
  return *this;
}

void StartupBuilder::receiveSignal(int signal) {
  switch (signal) {
  case SIGINT:
  case SIGCHLD:
  case SIGTERM:
    startupBuilder->onShutdown();
    break;
  default:
    LOG_ERROR("{} undefined signal", signal);
    break;
  }
}

void StartupBuilder::onShutdown() {
  LOG_INFO("{} shutdown", processInfo->name);

  conditionVariable.notify_all();
  // trigger shutdown
  for (auto &&abstractService : abstractServices) {
    abstractService->onShutdown();
  }
}