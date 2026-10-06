#include "base_library/core/StartupBuilder.h"

#include <base_library/features/base/configuration/EnvironmentConfiguration.h>

#include <algorithm>
#include <csignal>
#include <memory>
#include <thread>
#include <utility>

#include "base_library/features/Feature.h"

#include "Hypodermic/Container.h"
#include "Hypodermic/ContainerBuilder.h"

#include "base_library/core/services/BootstrapService.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/core/services/PersistableService.h"
#include "base_library/features/base/configuration/Component.h"
#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/configuration/ConfigurationComponentBuilder.h"
#include "base_library/features/base/models/ProcessName.h"
#include "base_library/features/base/services/InitializeService.h"
#include "base_library/features/base/services/ProcessArgumentService.h"

StartupBuilder::StartupBuilder(ProcessName &&name,
                               std::vector<std::string> &&arguments)
        : m_name(std::make_shared<ProcessName>(std::move(name))),
          m_arguments(std::move(arguments)),
          m_environmentConfiguration(std::make_shared<EnvironmentConfiguration>()),
          m_configurationComponentBuilder(
                  std::make_shared<ConfigurationComponentBuilder>(
                          m_environmentConfiguration)) {
}

void StartupBuilder::addConfigurationComponent(
        std::shared_ptr<Component> &&component) {
    m_configurationComponentBuilder->add(std::move(component));
}

// argv decays to char** in the parameter list, so the pointer form is the same
// type as the array form used in the declaration
std::shared_ptr<StartupBuilder> StartupBuilder::with(int argc, char **argv) {
    // I process Information
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
    return builder;
}

void StartupBuilder::withOutFeature(std::string_view nameOfFeature) {
    m_featureVec.erase(std::remove_if(m_featureVec.begin(), m_featureVec.end(),
                                      [&nameOfFeature](auto &&feature) -> bool {
                                          return feature->getName() == nameOfFeature;
                                      }),
                       m_featureVec.end());
}

void StartupBuilder::overrides(EnvironmentConfiguration::Environment environment, std::string value) {
  // std::move vermeidet die zweite Kopie: EnvironmentConfiguration::overrides
  // nimmt den Wert selbst per value und bewegt ihn in seine Map.
    m_environmentConfiguration->overrides(environment, std::move(value));
}

void StartupBuilder::start() {
    // I injection
    Hypodermic::ContainerBuilder builder;
    m_configuration = std::make_shared<Configuration>(
            m_configurationComponentBuilder->build(), m_environmentConfiguration);
    builder.registerInstance(m_configuration);
    builder.registerInstance(m_name);
    builder.registerInstance(m_environmentConfiguration);
    // the configuration and process name are available to features during
    // registration so that services can be registered conditionally
    // (e.g. HistoryService only in its explicitly configured owner process)
    for (auto &&feature: m_featureVec) {
        feature->registerTypes(builder, m_configuration, m_name);
    }
    // II logger
    DECLARE_LOGGER(m_name, m_configuration);
    // III dedicated signal handling thread (sigwait instead of std::signal)
    sigset_t signalSet;
    sigemptyset(&signalSet);
    sigaddset(&signalSet, SIGINT);
    sigaddset(&signalSet, SIGCHLD);
    sigaddset(&signalSet, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &signalSet, nullptr);
    try {
        // IV start IOC Container build
        m_container = builder.build();
        // IV boostrap plugins trigger initialization
        if (m_bootStrapServiceActive) {
            std::shared_ptr<BootstrapService> bootstrapService =
                    m_container->resolve<BootstrapService>();
            bootstrapService->onStart();
        }
        // V set AbstractService for shutdown event (before the signal thread
        // starts, so onShutdown never reads the vector unsynchronized)
        m_abstractServices = m_container->resolveAll<AbstractServiceInterface>();
        m_signalThread = std::thread(&StartupBuilder::signalThreadLoop, this);
        // VI start services
        for (auto &&feature: m_featureVec) {
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
        initializeService->onInitialize();
        LOG_INFO("{} finished initialization", m_name->getProcessName());
        // IX wait for signal to shutdown
        std::unique_lock<std::mutex> lock(mutex);
        m_conditionVariable.wait(lock, [&]() -> bool { return m_stop; });
        if (m_signalThread.joinable()) {
            m_signalThread.join();
        }
    } catch (...) {
        // never leave the signal thread joinable (std::terminate in the
        // destructor) and restore the signal mask so the process can still
        // be stopped if the startup failed before the thread was created
        m_stop.store(true);
        m_conditionVariable.notify_all();
        if (m_signalThread.joinable()) {
            m_signalThread.join();
        }
        pthread_sigmask(SIG_UNBLOCK, &signalSet, nullptr);
        throw;
    }
}

void StartupBuilder::signalThreadLoop() {
    sigset_t signalSet;
    sigemptyset(&signalSet);
    sigaddset(&signalSet, SIGINT);
    sigaddset(&signalSet, SIGCHLD);
    sigaddset(&signalSet, SIGTERM);

    int sig = 0;
    while (true) {
        int result = sigwait(&signalSet, &sig);
        if (result != 0)
            continue;

        switch (sig) {
            case SIGCHLD:
                break;
            case SIGINT:
            case SIGTERM:
                onShutdown();
                return;
            default:
                break;
        }
    }
}

void StartupBuilder::onShutdown() {
    m_stop.store(true);
    LOG_INFO("{} shutdown", m_name->getProcessName());
    // trigger shutdown
    std::sort(m_abstractServices.begin(), m_abstractServices.end(),
              [](const std::shared_ptr<AbstractServiceInterface> &a,
                 const std::shared_ptr<AbstractServiceInterface> &b) {
                  return a->shutdownPriorityOf() < b->shutdownPriorityOf();
              });
    for (const auto &abstractService: m_abstractServices) {
        LOG_TRACE("shutdown of {}", abstractService->getClassName());
        abstractService->onShutdown();
    }
    m_conditionVariable.notify_all();
}

void StartupBuilder::disableBootstrapService() {
    m_bootStrapServiceActive = false;
}
