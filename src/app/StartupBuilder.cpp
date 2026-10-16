#include "lintel/app/StartupBuilder.h"

#include <lintel/core/configuration/EnvironmentConfiguration.h>

#include <algorithm>
#include <csignal>
#include <memory>
#include <thread>
#include <utility>

#if defined(_WIN32)
// Keep the transitive Windows headers from defining the min/max macros.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>// SetConsoleCtrlHandler
#else
#include <pthread.h>// pthread_sigmask / sigwait
#endif

#include "lintel/core/Feature.h"

#include "lintel/core/di/Container.h"

#include "lintel/app/ConfigurationComponentBuilder.h"
#include "lintel/core/configuration/Component.h"
#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/models/ProcessName.h"
#include "lintel/core/services/BootstrapService.h"
#include "lintel/core/services/LoggerService.h"
#include "lintel/core/services/PersistableService.h"
#include "lintel/features/base/services/InitializeService.h"
#include "lintel/features/base/services/ProcessArgumentService.h"

namespace lintel {
#if defined(_WIN32)
namespace {
  /**
   * The builder currently inside start(). SetConsoleCtrlHandler and the CRT
   * signal facilities accept no user-data pointer, so the handler entry points
   * reach the active builder through this slot.
   */
  std::atomic<StartupBuilder *> g_activeBuilder = nullptr;

  /**
   * Guards the single Windows onShutdown() execution: the console control
   * handler, the CRT signal handler and a concurrently delivered Ctrl+C can all
   * request the shutdown for the same event.
   */
  std::atomic_bool g_shutdownClaimed = false;

  /**
   * @return the builder whose shutdown may be claimed, or nullptr when there is
   *         no active builder or another handler already claimed the shutdown
   */
  StartupBuilder *claimShutdown()
  {
    StartupBuilder *builder = g_activeBuilder.load();
    if (builder == nullptr) { return nullptr; }
    if (g_shutdownClaimed.exchange(true)) { return nullptr; }
    return builder;
  }

  /**
   * SetConsoleCtrlHandler entry point. Ctrl+C, Ctrl+Break, console close, logoff
   * and system shutdown map onto the graceful shutdown that POSIX performs on
   * SIGINT/SIGTERM. The handler runs on an OS-created thread, so onShutdown() -
   * including its service thread joins - can run directly here.
   */
  BOOL WINAPI consoleCtrlHandler(DWORD ctrlType)
  {
    static_cast<void>(ctrlType);
    if (StartupBuilder *builder = claimShutdown()) { builder->onShutdown(); }
    return TRUE;
  }

  /**
   * std::signal(SIGINT/SIGBREAK) entry point. The CLI calls std::raise(SIGINT)
   * itself (login failure, "exit" command); on Windows that dispatches the
   * handler synchronously on the calling input thread, and
   * CommandLineService::onShutdown() joins exactly that thread, so the shutdown
   * has to run on a different one.
   */
  extern "C" void consoleSignalHandler(int signal)
  {
    static_cast<void>(signal);
    if (StartupBuilder *builder = claimShutdown()) {
      std::thread([builder] { builder->onShutdown(); }).detach();
    }
  }
}// namespace
#endif

StartupBuilder::StartupBuilder(ProcessName &&name, std::vector<std::string> &&arguments)
  : m_name(std::make_shared<ProcessName>(std::move(name))), m_arguments(std::move(arguments)),
    m_environmentConfiguration(std::make_shared<EnvironmentConfiguration>()),
    m_configurationComponentBuilder(std::make_shared<ConfigurationComponentBuilder>(m_environmentConfiguration))
{}

void StartupBuilder::addConfigurationComponent(std::shared_ptr<Component> &&component)
{
  m_configurationComponentBuilder->add(std::move(component));
}

std::shared_ptr<StartupBuilder> StartupBuilder::with(int argc, char **argv)
{
  // I process Information
  ProcessName name(argc, argv);
  // II arguments
  std::vector<std::string> arguments;
  // III ignore name of process => start with 1
  for (int i = 1; i < argc; i++) { arguments.emplace_back(argv[i]); }
  // IV instantiate startupbuilder
  std::shared_ptr<StartupBuilder> builder = std::make_shared<StartupBuilder>(std::move(name), std::move(arguments));
  return builder;
}

void StartupBuilder::withOutFeature(std::string_view nameOfFeature)
{
  m_featureVec.erase(std::remove_if(m_featureVec.begin(),
                       m_featureVec.end(),
                       [&nameOfFeature](auto &&feature) -> bool { return feature->getName() == nameOfFeature; }),
    m_featureVec.end());
}

void StartupBuilder::overrides(EnvironmentConfiguration::Environment environment, std::string value)
{
  // std::move avoids the second copy: EnvironmentConfiguration::overrides
  // takes the value by value itself and moves it into its map.
  m_environmentConfiguration->overrides(environment, std::move(value));
}

void StartupBuilder::start()
{
  // I injection
  lintel::di::ContainerBuilder builder;
  m_configuration =
    std::make_shared<Configuration>(m_configurationComponentBuilder->build(), m_environmentConfiguration);
  builder.registerInstance(m_configuration);
  builder.registerInstance(m_name);
  builder.registerInstance(m_environmentConfiguration);
  // the configuration and process name are available to features during
  // registration so that services can be registered conditionally
  // (e.g. HistoryService only in its explicitly configured owner process)
  for (auto &&feature : m_featureVec) { feature->registerTypes(builder, m_configuration, m_name); }
  // II logger
  DECLARE_LOGGER(m_name, m_configuration);
  // III dedicated signal handling thread (sigwait instead of std::signal)
#if defined(_WIN32)
  // Windows has no sigwait()/SIGCHLD: console control events (Ctrl+C, Ctrl+Break,
  // console close) reach consoleCtrlHandler, and the CLI's std::raise(SIGINT)
  // reaches consoleSignalHandler; both run the same graceful shutdown.
  g_shutdownClaimed.store(false);
  g_activeBuilder.store(this);
  static_cast<void>(SetConsoleCtrlHandler(consoleCtrlHandler, TRUE));
  static_cast<void>(std::signal(SIGINT, consoleSignalHandler));
  static_cast<void>(std::signal(SIGBREAK, consoleSignalHandler));
#else
  sigset_t signalSet;
  sigemptyset(&signalSet);
  sigaddset(&signalSet, SIGINT);
  sigaddset(&signalSet, SIGCHLD);
  sigaddset(&signalSet, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &signalSet, nullptr);
#endif
  try {
    // IV start IOC Container build
    m_container = builder.build();
    // IV boostrap plugins trigger initialization
    if (m_bootStrapServiceActive) {
      std::shared_ptr<BootstrapService> bootstrapService = m_container->resolve<BootstrapService>();
      bootstrapService->onStart();
    }
    // V set AbstractService for shutdown event (before the signal thread
    // starts, so onShutdown never reads the vector unsynchronized)
    m_abstractServices = m_container->resolveAll<AbstractServiceInterface>();
    m_signalThread = std::thread(&StartupBuilder::signalThreadLoop, this);
    // VI start services
    for (auto &&feature : m_featureVec) { feature->initialize(m_container); }
    std::shared_ptr<ProcessArgumentService> processArgumentService = m_container->resolve<ProcessArgumentService>();
    processArgumentService->parseArguments(m_arguments);
    // VII awake all from persistence
    std::shared_ptr<PersistableService> persistableService = m_container->resolve<PersistableService>();
    persistableService->awake();
    // VIII initialize services
    std::shared_ptr<InitializeService> initializeService = m_container->resolve<InitializeService>();
    initializeService->onInitialize();
    LOG_INFO("{} finished initialization", m_name->getProcessName());
    // IX wait for signal to shutdown
    std::unique_lock<std::mutex> lock(mutex);
    m_conditionVariable.wait(lock, [&]() -> bool { return m_stop; });
    if (m_signalThread.joinable()) { m_signalThread.join(); }
#if defined(_WIN32)
    g_activeBuilder.store(nullptr);
    static_cast<void>(SetConsoleCtrlHandler(consoleCtrlHandler, FALSE));
#endif
  } catch (...) {
    // never leave the signal thread joinable (std::terminate in the
    // destructor) and restore the signal mask so the process can still
    // be stopped if the startup failed before the thread was created
    m_stop.store(true);
    m_conditionVariable.notify_all();
    if (m_signalThread.joinable()) { m_signalThread.join(); }
#if defined(_WIN32)
    g_activeBuilder.store(nullptr);
    static_cast<void>(SetConsoleCtrlHandler(consoleCtrlHandler, FALSE));
#else
    pthread_sigmask(SIG_UNBLOCK, &signalSet, nullptr);
#endif
    throw;
  }
}

void StartupBuilder::signalThreadLoop()
{
#if defined(_WIN32)
  // Windows has no sigwait()/SIGINT/SIGCHLD: console control events are handled
  // by consoleCtrlHandler and the CLI's std::raise(SIGINT) by consoleSignalHandler
  // (both above), so this thread has nothing to wait for and returns immediately;
  // start() still joins it. boost::process child reaping needs no SIGCHLD here
  // because ProcessService polls child exit on Windows.
  return;
#else
  sigset_t signalSet;
  sigemptyset(&signalSet);
  sigaddset(&signalSet, SIGINT);
  sigaddset(&signalSet, SIGCHLD);
  sigaddset(&signalSet, SIGTERM);

  int sig = 0;
  while (true) {
    int result = sigwait(&signalSet, &sig);
    if (result != 0) continue;

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
#endif
}

void StartupBuilder::onShutdown()
{
  {
    // flip m_stop under the mutex start() waits under, so the notify_all below
    // cannot be lost between the predicate check and the wait
    std::lock_guard<std::mutex> lock(mutex);
    m_stop.store(true);
  }
  LOG_INFO("{} shutdown", m_name->getProcessName());
  // trigger shutdown
  std::sort(m_abstractServices.begin(),
    m_abstractServices.end(),
    [](const std::shared_ptr<AbstractServiceInterface> &a, const std::shared_ptr<AbstractServiceInterface> &b) {
      return a->shutdownPriorityOf() < b->shutdownPriorityOf();
    });
  for (const auto &abstractService : m_abstractServices) {
    LOG_TRACE("shutdown of {}", abstractService->getClassName());
    abstractService->onShutdown();
  }
  m_conditionVariable.notify_all();
}

void StartupBuilder::disableBootstrapService() { m_bootStrapServiceActive = false; }
}// namespace lintel
