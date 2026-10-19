#ifndef LINTEL_STARTUPBUILDER_H
#define LINTEL_STARTUPBUILDER_H

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "lintel/core/configuration/EnvironmentConfiguration.h"

namespace lintel {
class AbstractServiceInterface;
class Component;
class Configuration;
class ConfigurationComponentBuilder;
class FeatureInterface;
class Features;
class ProcessName;

}// namespace lintel
namespace lintel {
namespace di {
  class Container;
}// namespace di
}// namespace lintel

namespace lintel {
/** Builder for application startup lifecycle, managing features, configuration, and signal handling. */
class StartupBuilder
{
private:
  std::vector<std::shared_ptr<FeatureInterface>> m_featureVec;
  std::shared_ptr<lintel::di::Container> m_container = nullptr;
  std::shared_ptr<ProcessName> m_name;
  std::vector<std::shared_ptr<AbstractServiceInterface>> m_abstractServices;
  std::thread m_signalThread;
  std::condition_variable m_conditionVariable;
  std::atomic_bool m_stop = false;
  std::vector<std::string> m_arguments;
  std::shared_ptr<EnvironmentConfiguration> m_environmentConfiguration = nullptr;
  std::shared_ptr<ConfigurationComponentBuilder> m_configurationComponentBuilder;
  std::shared_ptr<Configuration> m_configuration = nullptr;
  std::shared_ptr<Features> m_features = std::make_shared<Features>();
  bool m_bootStrapServiceActive = true;
  std::mutex mutex;

  /** Verify that every feature's required features were added before it.
   *
   * Throws std::runtime_error naming the offending feature when a required
   * feature is missing or was added later (the layers form a linear chain).
   * Called by start() before any type is registered, so a broken feature set
   * fails fast instead of degrading silently. */
  void verifyFeatureDependencies() const;

public:
  /** Construct a StartupBuilder with a process name and command-line arguments.
   * @param processName the name identifying this process
   * @param arguments   the command-line arguments */
  StartupBuilder(ProcessName &&processName, std::vector<std::string> &&arguments);

  StartupBuilder() = delete;

  /** Factory method to create a StartupBuilder from argc/argv.
   * @param argc argument count
   * @param argv argument vector
   * @return shared pointer to the new StartupBuilder */
  static std::shared_ptr<StartupBuilder> with(int argc, char **argv);

  /** Register a configuration component.
   * @param component the component to add */
  void addConfigurationComponent(std::shared_ptr<Component> &&component);

  /** Register a feature by its type. The feature is instantiated with the shared feature set.
   * @tparam FEATURE the feature type to add */
  template<typename FEATURE> void addFeature() { m_featureVec.push_back(std::make_shared<FEATURE>(m_features)); }

  /** Disable the bootstrap service on startup. */
  void disableBootstrapService();

  /** Exclude a named feature from the startup sequence.
   * @param nameOfFeature the feature name to exclude */
  void withOutFeature(std::string_view nameOfFeature);

  /** Override an environment configuration value.
   * @param environment the environment to override
   * @param value       the override value */
  void overrides(EnvironmentConfiguration::Environment environment, std::string value);

  /** Perform shutdown logic, notifying all registered services. */
  void onShutdown();

  /** Internal signal-handling thread loop. */
  void signalThreadLoop();

  /** Start all features, services, and begin the application main loop. */
  void start();
};

}// namespace lintel
#endif// LINTEL_STARTUPBUILDER_H
