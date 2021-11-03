#include "base_library/core/StartupBuilder.h"

#include <base_library/features/base/configuration/EnvironmentConfiguration.h>

#include <algorithm>
#include <csignal>
#include <memory>

#include "base_library/core/services/BootstrapService.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/core/services/PersistableService.h"
#include "base_library/core/services/ProcessService.h"
#include "base_library/features/base/configuration/Component.h"
#include "base_library/features/base/configuration/ConfigurationComponentBuilder.h"
#include "base_library/features/base/models/ProcessName.h"
#include "base_library/features/base/services/InitializeService.h"
#include "base_library/features/base/services/ProcessArgumentService.h"

StartupBuilder *StartupBuilder::m_startupBuilder = nullptr;

StartupBuilder::StartupBuilder(ProcessName &&name,
                               std::vector<std::string> &&arguments)
    : m_name(std::make_shared<ProcessName>(std::move(name))),
      m_arguments(std::move(arguments)),
      m_configurationComponentBuilder(
          std::make_shared<ConfigurationComponentBuilder>()),
      m_environmentConfiguration(std::make_shared<EnvironmentConfiguration>()) {
  signal(SIGINT, StartupBuilder::receiveSignal);
  signal(SIGCHLD, StartupBuilder::receiveSignal);
  signal(SIGTERM, StartupBuilder::receiveSignal);
}

void StartupBuilder::addConfigurationComponent(
    std::shared_ptr<Component> &&component) {
  m_configurationComponentBuilder->add(std::move(component));
}

std::shared_ptr<StartupBuilder> StartupBuilder::with(int argc, char *argv[]) {
  // I process Informations
  ProcessName name(argc, argv);
  // II arguments
  std::vector<std::string> arguments;
  // III ignore name of process => start with 1
  for (int i = 1; i < argc; i++) {
    arguments.emplace_back(argv[i]);
  }
  // IV instantiate startupbuilder
  std::shared_ptr<StartupBuilder> builder =
      std::make_shared<StartupBuilder>(std::move(name), std::move(arguments));
  m_startupBuilder = builder.get();
  return builder;
}

void StartupBuilder::withOutFeature(std::string_view nameOfFeature) {
  m_features.erase(std::remove_if(m_features.begin(), m_features.end(),
                                  [&nameOfFeature](auto &&feature) -> bool {
                                    return feature->getName() == nameOfFeature;
                                  }),
                   m_features.end());
}

void StartupBuilder::start() {
  // I injection
  Hypodermic::ContainerBuilder builder;
  for (auto &&feature : m_features) {
    feature->registerTypes(builder);
  }
  builder.registerInstance(m_name);
  builder.registerInstance(m_environmentConfiguration);
  m_configuration = std::make_shared<Configuration>(
      m_configurationComponentBuilder->build(), m_environmentConfiguration);
  builder.registerInstance(m_configuration);
  // II logger
  DECLARE_LOGGER(m_name, m_configuration);
  // III start IOC Container build
  m_container = builder.build();
  // IV boostrap plugins trigger initialization
  if (m_bootStrapServiceActive) {
    std::shared_ptr<BootstrapService> bootstrapService =
        m_container->resolve<BootstrapService>();
    bootstrapService->onStart();
  }
  // V set AbstractService for shutdown event
  m_abstractServices = m_container->resolveAll<AbstractServiceInterface>();
  // VI start services
  for (auto &&feature : m_features) {
    feature->initialize(m_container);
  }
  std::shared_ptr<ProcessArgumentService> processArgumentService =
      m_container->resolve<ProcessArgumentService>();
  processArgumentService->parseArguments(m_arguments);
  // VII awake all from persistence
  std::shared_ptr<PersistableService> persistableService =
      m_container->resolve<PersistableService>();
  persistableService->awake();
  // VIII initialize services
  std::shared_ptr<InitializeService> initializeService =
      m_container->resolve<InitializeService>();
  LOG_INFO("{} finished initialization", m_name->getProcessName());
  // IX wait for signal to shutdown
  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  m_conditionVariable.wait(lock);
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
  LOG_INFO("{} shutdown", m_name->getProcessName());
  // trigger shutdown
  for (const auto &abstractService : m_abstractServices) {
    LOG_TRACE("shutdown of {}", abstractService->getClassName());
    abstractService->onShutdown();
  }
  m_conditionVariable.notify_all();
}
void StartupBuilder::disableBootstrapService() {
  m_bootStrapServiceActive = false;
}
