#include <lintel/app/StartupBuilder.h>

#include <catch2/catch_all.hpp>

#include <lintel/core/Feature.h>
#include <lintel/core/Features.h>

#include <lintel/app/BaseFeature.h>
#include <lintel/app/CommandLineFeature.h>
#include <lintel/app/HttpFeature.h>
#include <lintel/app/PropertyFeature.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#if !defined(_WIN32)
#include <pthread.h>// pthread_sigmask
#endif

#include "lintel/core/di/Container.h"
#include "lintel/core/models/ShutdownPriority.h"
#include "lintel/core/services/AbstractService.h"

#include "../helpers/PlatformCompat.h"
#include "../helpers/ScopedEnvironmentVariable.h"

namespace lintel {
namespace {

  // `sb` prefix: unity builds merge several test files into one translation unit
  // and share one anonymous namespace, so a generic `featureLog` or `builder`
  // would collide (and shadow local variables, Clang -Wshadow).
  constexpr char sbProcessArgv0[] = "startup_builder_test_process";

  /**
   * Order in which the test features reached StartupBuilder::start().
   *
   * A file-scope log rather than a fixture member, because addFeature<FEATURE>()
   * instantiates the feature with the shared feature set only - there is no place
   * to hand a per-test-case sink into it.
   * @return the mutable registration log, shared by all features of this file */
  std::vector<std::string> &sbFeatureLog()
  {
    static std::vector<std::string> log;
    return log;
  }

  /**
   * Minimal feature that records when it was registered with the DI container.
   *
   * The DI container stays an incomplete type: ContainerBuilder is only
   * passed by reference and Container travels inside a shared_ptr, exactly as
   * in lintel/core/Feature.h.
   */
  class SbCountingFeature : public FeatureInterface
  {
  public:
    SbCountingFeature(std::shared_ptr<Features> /*features*/, std::string name) : m_name(std::move(name)) {}

    void registerTypes(lintel::di::ContainerBuilder & /*builder*/) override { sbFeatureLog().push_back(m_name); }

    void initialize(std::shared_ptr<lintel::di::Container> /*container*/) override {}

    std::string_view getName() override { return m_name; }

  private:
    std::string m_name;
  };

  // StartupBuilder::addFeature<FEATURE>() constructs FEATURE(features), so every feature needs its own
  // type even though the three only differ in the name they report.
  class SbFeatureA : public SbCountingFeature
  {
  public:
    explicit SbFeatureA(std::shared_ptr<Features> features) : SbCountingFeature(features, "SbFeatureA") {}
  };

  class SbFeatureB : public SbCountingFeature
  {
  public:
    explicit SbFeatureB(std::shared_ptr<Features> features) : SbCountingFeature(features, "SbFeatureB") {}
  };

  class SbFeatureC : public SbCountingFeature
  {
  public:
    explicit SbFeatureC(std::shared_ptr<Features> features) : SbCountingFeature(features, "SbFeatureC") {}
  };

  /** Feature that declares a dependency on SbFeatureA, to exercise the layer check. */
  class SbDependentFeature : public SbCountingFeature
  {
  public:
    explicit SbDependentFeature(std::shared_ptr<Features> features) : SbCountingFeature(features, "SbDependentFeature")
    {}

    std::vector<std::string_view> requiredFeatures() const override { return { "SbFeatureA" }; }
  };

  /**
   * @param arguments the arguments after argv[0]
   * @return a StartupBuilder built from a synthetic argc/argv
   *
   * The strings are copied into a vector that stays alive for the call, so the builder gets a valid
   * argv without the test case having to keep arrays around.
   */
  std::shared_ptr<StartupBuilder> sbMakeBuilder(const std::vector<std::string> &arguments = {})
  {
    std::vector<std::string> storage;
    storage.push_back(sbProcessArgv0);
    for (const auto &argument : arguments) { storage.push_back(argument); }
    std::vector<char *> argv;
    argv.reserve(storage.size());
    for (auto &value : storage) { argv.push_back(value.data()); }
    return StartupBuilder::with(static_cast<int>(argv.size()), argv.data());
  }

  /**
   * Isolates the builder from the checkout's cfg/ directory.
   *
   * StartupBuilder reads CONFIG_DIRECTORY and BOOTSTRAP_CONFIG_NAME in its own constructor, so the
   * guard has to be alive before sbMakeBuilder() is called. Every test case declares it as its first
   * statement for that reason.
   */
  void sbForgetConfiguration() { sbFeatureLog().clear(); }

  /** Instance names of the lifecycle services in the order they were shut down. */
  std::vector<std::string> &sbShutdownOrder()
  {
    static std::vector<std::string> order;
    return order;
  }

  /** Number of lifecycle services that have been initialized. */
  std::atomic<int> &sbInitializedCount()
  {
    static std::atomic<int> count{ 0 };
    return count;
  }

  /**
   * Lifecycle service that records its initialization and shutdown.
   *
   * A file-scope log instead of a fixture member, because the DI container constructs the services
   * from a type registration - there is no place to hand a per-test-case sink into them.
   */
  class SbLifecycleService : public AbstractService<SbLifecycleService>
  {
  public:
    SbLifecycleService(std::string instanceName, ShutdownPriority priority)
      : AbstractService<SbLifecycleService>("startup_builder_test_process", std::move(instanceName)),
        m_priority(priority)
    {}

    void onInitialize() override { ++sbInitializedCount(); }

    void onShutdown() override { sbShutdownOrder().push_back(getInstanceName()); }

    ShutdownPriority shutdownPriorityOf() const override { return m_priority; }

  private:
    ShutdownPriority m_priority;
  };

  /** Default shutdown priority, i.e. shut down before the process service. */
  class SbDefaultPriorityService : public SbLifecycleService
  {
  public:
    SbDefaultPriorityService() : SbLifecycleService("default_priority_service", ShutdownPriority::DEFAULT) {}
  };

  /** Process service priority, i.e. shut down last. */
  class SbProcessPriorityService : public SbLifecycleService
  {
  public:
    SbProcessPriorityService() : SbLifecycleService("process_priority_service", ShutdownPriority::PROCESS_SERVICE) {}
  };

  /** Registers the two lifecycle services of the shutdown order test. */
  class SbLifecycleFeature : public FeatureInterface
  {
  public:
    explicit SbLifecycleFeature(std::shared_ptr<Features> /*features*/) {}

    void registerTypes(lintel::di::ContainerBuilder &builder) override
    {
      builder.registerType<SbDefaultPriorityService>().as<AbstractServiceInterface>().singleInstance();
      builder.registerType<SbProcessPriorityService>().as<AbstractServiceInterface>().singleInstance();
    }

    void initialize(std::shared_ptr<lintel::di::Container> /*container*/) override {}

    std::string_view getName() override { return "SbLifecycleFeature"; }
  };

  /**
   * Blocks the shutdown signals in the calling thread for the test duration.
   *
   * start() blocks SIGINT/SIGCHLD/SIGTERM in the thread that calls it and waits for them in a
   * sigwait() thread. With the same signals blocked in the test thread as well, the SIGTERM below
   * stays pending until that sigwait() thread consumes it - without the guard its default action
   * would kill the test process. The previous mask is restored on destruction, so the test cases
   * behind this one are not affected.
   */
  struct SbSignalMaskGuard
  {
#if !defined(_WIN32)
    sigset_t m_previous{};
    sigset_t m_blocked{};
#endif

    SbSignalMaskGuard()
    {
#if !defined(_WIN32)
      sigemptyset(&m_blocked);
      sigaddset(&m_blocked, SIGINT);
      sigaddset(&m_blocked, SIGCHLD);
      sigaddset(&m_blocked, SIGTERM);
      pthread_sigmask(SIG_BLOCK, &m_blocked, &m_previous);
#endif
    }

    ~SbSignalMaskGuard()
    {
#if !defined(_WIN32)
      pthread_sigmask(SIG_SETMASK, &m_previous, nullptr);
#endif
    }

    SbSignalMaskGuard(const SbSignalMaskGuard &) = delete;
    SbSignalMaskGuard &operator=(const SbSignalMaskGuard &) = delete;
  };

  /** Joins the start() thread on destruction, so a failing REQUIRE cannot terminate the process
   *  through a joinable std::thread. */
  struct SbStarterGuard
  {
    std::thread &thread;

    ~SbStarterGuard()
    {
      if (thread.joinable()) { thread.join(); }
    }
  };

}// namespace

TEST_CASE("StartupBuilder::with creates a builder from argc/argv", "[startup_builder]")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_startup_builder_test_cfg" };
  ScopedEnvironmentVariable bootstrapName{ "BOOTSTRAP_CONFIG_NAME", "no_such_bootstrap" };
  sbForgetConfiguration();

  auto withoutArguments = sbMakeBuilder();
  REQUIRE(withoutArguments != nullptr);

  auto withArguments = sbMakeBuilder({ "--verbose", "--config", "x" });
  REQUIRE(withArguments != nullptr);
  REQUIRE(withArguments.get() != withoutArguments.get());

  // both builders are usable: they run the registration phase of start() and only then stop at the
  // logger phase, because the configuration directory holds no bootstrap configuration
  REQUIRE_THROWS_AS(withoutArguments->start(), std::runtime_error);
  REQUIRE_THROWS_WITH(withArguments->start(), Catch::Matchers::ContainsSubstring("logger path"));
}

TEST_CASE("StartupBuilder::start registers every feature in the order it was added", "[startup_builder]")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_startup_builder_test_cfg" };
  ScopedEnvironmentVariable bootstrapName{ "BOOTSTRAP_CONFIG_NAME", "no_such_bootstrap" };
  sbForgetConfiguration();

  auto builder = sbMakeBuilder();
  REQUIRE(builder != nullptr);

  builder->addFeature<SbFeatureA>();
  builder->addFeature<SbFeatureB>();
  builder->addFeature<SbFeatureC>();

  // phase I registers the types of every feature, phase II builds the logger and fails
  REQUIRE_THROWS_WITH(builder->start(), Catch::Matchers::ContainsSubstring("logger path"));

  const std::vector<std::string> expected{ "SbFeatureA", "SbFeatureB", "SbFeatureC" };
  REQUIRE(sbFeatureLog() == expected);
}

TEST_CASE("StartupBuilder::withOutFeature removes a feature from the startup sequence", "[startup_builder]")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_startup_builder_test_cfg" };
  ScopedEnvironmentVariable bootstrapName{ "BOOTSTRAP_CONFIG_NAME", "no_such_bootstrap" };
  sbForgetConfiguration();

  auto builder = sbMakeBuilder();
  REQUIRE(builder != nullptr);

  builder->addFeature<SbFeatureA>();
  builder->addFeature<SbFeatureB>();
  builder->addFeature<SbFeatureC>();
  builder->withOutFeature("SbFeatureB");
  // a name that was never added has to be a no-op rather than an error
  builder->withOutFeature("SbFeatureThatWasNeverAdded");

  REQUIRE_THROWS_WITH(builder->start(), Catch::Matchers::ContainsSubstring("logger path"));

  const std::vector<std::string> expected{ "SbFeatureA", "SbFeatureC" };
  REQUIRE(sbFeatureLog() == expected);
}

TEST_CASE("StartupBuilder::start rejects a feature whose required feature was never added", "[startup_builder]")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_startup_builder_test_cfg" };
  ScopedEnvironmentVariable bootstrapName{ "BOOTSTRAP_CONFIG_NAME", "no_such_bootstrap" };
  sbForgetConfiguration();

  auto builder = sbMakeBuilder();
  REQUIRE(builder != nullptr);
  builder->addFeature<SbDependentFeature>();

  // the dependency check runs before phase I, so nothing was registered yet
  REQUIRE_THROWS_AS(builder->start(), std::runtime_error);
  REQUIRE_THROWS_WITH(builder->start(), Catch::Matchers::ContainsSubstring("requires 'SbFeatureA'"));
  REQUIRE(sbFeatureLog().empty());
}

TEST_CASE("StartupBuilder::start rejects a required feature added after its dependent", "[startup_builder]")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_startup_builder_test_cfg" };
  ScopedEnvironmentVariable bootstrapName{ "BOOTSTRAP_CONFIG_NAME", "no_such_bootstrap" };
  sbForgetConfiguration();

  auto builder = sbMakeBuilder();
  REQUIRE(builder != nullptr);
  builder->addFeature<SbDependentFeature>();
  builder->addFeature<SbFeatureA>();

  REQUIRE_THROWS_AS(builder->start(), std::runtime_error);
  REQUIRE_THROWS_WITH(
    builder->start(), Catch::Matchers::ContainsSubstring("must be added after its required feature 'SbFeatureA'"));
  REQUIRE(sbFeatureLog().empty());
}

TEST_CASE("StartupBuilder::withOutFeature makes the dependent feature fail fast", "[startup_builder]")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_startup_builder_test_cfg" };
  ScopedEnvironmentVariable bootstrapName{ "BOOTSTRAP_CONFIG_NAME", "no_such_bootstrap" };
  sbForgetConfiguration();

  auto builder = sbMakeBuilder();
  REQUIRE(builder != nullptr);
  builder->addFeature<SbFeatureA>();
  builder->addFeature<SbDependentFeature>();
  builder->withOutFeature("SbFeatureA");

  REQUIRE_THROWS_WITH(builder->start(), Catch::Matchers::ContainsSubstring("requires 'SbFeatureA'"));
  REQUIRE(sbFeatureLog().empty());
}

TEST_CASE("StartupBuilder::start accepts a dependency that is satisfied before its dependent", "[startup_builder]")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_startup_builder_test_cfg" };
  ScopedEnvironmentVariable bootstrapName{ "BOOTSTRAP_CONFIG_NAME", "no_such_bootstrap" };
  sbForgetConfiguration();

  auto builder = sbMakeBuilder();
  REQUIRE(builder != nullptr);
  builder->addFeature<SbFeatureA>();
  builder->addFeature<SbDependentFeature>();

  // the check passes, so the run gets as far as the missing logger configuration
  REQUIRE_THROWS_WITH(builder->start(), Catch::Matchers::ContainsSubstring("logger path"));

  const std::vector<std::string> expected{ "SbFeatureA", "SbDependentFeature" };
  REQUIRE(sbFeatureLog() == expected);
}

TEST_CASE("StartupBuilder::start reports a missing bootstrap configuration instead of hanging", "[startup_builder]")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_startup_builder_test_cfg" };
  ScopedEnvironmentVariable bootstrapName{ "BOOTSTRAP_CONFIG_NAME", "no_such_bootstrap" };
  sbForgetConfiguration();

  auto builder = sbMakeBuilder();
  REQUIRE(builder != nullptr);

  // Phase I builds the Configuration from the registered components; a missing file yields no
  // entries and no exception. Phase II then fails with a descriptive runtime_error - before the DI
  // container is built and before the signal thread is created, so a missing configuration can
  // neither hang nor terminate the process through a joinable std::thread.
  REQUIRE_THROWS_AS(builder->start(), std::runtime_error);
  REQUIRE_THROWS_WITH(builder->start(), "Please configure logger path");

  // nothing was registered and no container was built
  REQUIRE(sbFeatureLog().empty());
}

TEST_CASE("StartupBuilder::start shuts the services down in shutdown priority order", "[startup_builder]")
{
  // A configuration directory with a logger: the logger is built before any service, so start()
  // cannot get past phase II without one.
  const std::filesystem::path configDirectory =
    std::filesystem::temp_directory_path() / ("startup_builder_shutdown_" + std::to_string(::getpid()));
  std::filesystem::remove_all(configDirectory);
  std::filesystem::create_directories(configDirectory);
  {
    std::ofstream configFile(configDirectory / "bootstrap.xml");
    configFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                  "<Configuration>\n"
                  "  <Loggers>\n"
                  "    <Path path=\""
               << (configDirectory / "log").string()
               << "\" create_sub_dirs=\"true\"/>\n"
                  "    <Logger process_name=\".*\" level=\"info\" async=\"false\">\n"
                  "      <LoggerSink type=\"RotatingFileSink\" level=\"trace\" size=\"5MB\" max_files=\"10\"/>\n"
                  "    </Logger>\n"
                  "  </Loggers>\n"
                  "</Configuration>\n";
  }
  ScopedEnvironmentVariable configEnv{ "CONFIG_DIRECTORY", configDirectory.string() };
  ScopedEnvironmentVariable bootstrapEnv{ "BOOTSTRAP_CONFIG_NAME", "bootstrap" };

  sbForgetConfiguration();
  sbShutdownOrder().clear();
  sbInitializedCount().store(0);

  auto builder = sbMakeBuilder();
  REQUIRE(builder != nullptr);
  builder->addFeature<SbLifecycleFeature>();
  builder->disableBootstrapService();

  // signals stay blocked in this thread and in everything the start() thread spawns (see the guard)
  SbSignalMaskGuard signalGuard;

  std::exception_ptr failure;
  std::thread starter([&builder, &failure]() {
    try {
      builder->start();
    } catch (...) {
      failure = std::current_exception();
    }
  });
  SbStarterGuard starterGuard{ starter };

  // phase VIII initializes both services; stop as soon as they are up, so the signal cannot race
  // the initialization phase
  const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (sbInitializedCount().load() < 2 && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  if (sbInitializedCount().load() != 2) {
    // join first (also publishes `failure`), then report why start() did not get that far
    starterGuard.thread.join();
    if (failure != nullptr) { std::rethrow_exception(failure); }
  }
  REQUIRE(sbInitializedCount().load() == 2);

#if defined(_WIN32)
  // Windows has no sigwait() thread; the console control handler calls the same shutdown method
  builder->onShutdown();
#else
  REQUIRE(kill(getpid(), SIGTERM) == 0);
#endif
  starterGuard.thread.join();

  // ascending priority: the default services wind down first, the process service last, so it keeps
  // reaping children while the rest of the process is already stopping
  const std::vector<std::string> expected{ "default_priority_service", "process_priority_service" };
  REQUIRE(sbShutdownOrder() == expected);

  // best effort: the logger still holds a file below the directory
  std::error_code errorCode;
  std::filesystem::remove_all(configDirectory, errorCode);
}

TEST_CASE("lintel features declare their layer dependencies", "[startup_builder]")
{
  // Guards the concrete declarations: StartupBuilder only sees them through
  // FeatureInterface::requiredFeatures(), so a dropped override would silently
  // turn the layer check off again.
  auto features = std::make_shared<Features>();

  REQUIRE(BaseFeature(features).requiredFeatures().empty());
  REQUIRE(PropertyFeature(features).requiredFeatures() == std::vector<std::string_view>{ "Base" });
  REQUIRE(HttpFeature(features).requiredFeatures() == std::vector<std::string_view>{ "Property" });
  REQUIRE(CommandLineFeature(features).requiredFeatures() == std::vector<std::string_view>{ "Http" });
}
}// namespace lintel
