#include <lintel/core/StartupBuilder.h>

#include <catch2/catch_all.hpp>

#include <lintel/features/Feature.h>
#include <lintel/features/Features.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "../helpers/ScopedEnvironmentVariable.h"

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
 * Hypodermic stays an incomplete type: ContainerBuilder is only passed by
 * reference and Container travels inside a shared_ptr, exactly as in
 * lintel/features/Feature.h.
 */
class SbCountingFeature : public FeatureInterface
{
public:
  SbCountingFeature(std::shared_ptr<Features> /*features*/, std::string name) : m_name(std::move(name)) {}

  void registerTypes(Hypodermic::ContainerBuilder & /*builder*/) override { sbFeatureLog().push_back(m_name); }

  void initialize(std::shared_ptr<Hypodermic::Container> /*container*/) override {}

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
