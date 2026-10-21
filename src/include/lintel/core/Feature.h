#ifndef LINTEL_FEATURE_H
#define LINTEL_FEATURE_H

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "lintel/core/Features.h"
#include "lintel/core/utils/TypeName.h"

namespace lintel {
class Configuration;
class ProcessName;

// The DI container is only referenced through pointers/references in the
// signatures below, so an incomplete type is enough. Implementations that
// register or resolve types include "lintel/core/di/Container.h" themselves.
}// namespace lintel
namespace lintel {
namespace di {
  class Container;
  class ContainerBuilder;
}// namespace di
}// namespace lintel

namespace lintel {
/** Interface for all feature implementations */
class FeatureInterface
{
public:
  /** Virtual destructor: features are deleted through this interface */
  virtual ~FeatureInterface() = default;

  /** Register dependency injection types for this feature */
  virtual void registerTypes(lintel::di::ContainerBuilder &builder) = 0;

  /** Register dependency injection types for this feature.
   *
   * The configuration and process name are available at registration time
   * so that features can conditionally register types (e.g. a service that
   * is only created in the process explicitly configured as its owner).
   *
   * @param builder the DI container builder
   * @param configuration the parsed application configuration
   * @param processName the process name of the running process
   */
  virtual void registerTypes(lintel::di::ContainerBuilder &builder,
    const std::shared_ptr<Configuration> &configuration,
    const std::shared_ptr<ProcessName> &processName)
  {
    (void)configuration;
    (void)processName;
    registerTypes(builder);
  }

  /** Initialize the feature with the resolved container */
  virtual void initialize(std::shared_ptr<lintel::di::Container> container) = 0;

  /** Get the name of this feature */
  virtual std::string_view getName() = 0;

  /** Names of the features this feature depends on.
   *
   * StartupBuilder verifies before wiring the DI container that every required
   * feature was added and appears earlier in the feature list. The feature
   * layers form a linear chain (base -> property -> http -> cli), so a
   * positional check is enough; a missing or misordered dependency is reported
   * with a descriptive exception instead of degrading silently through the DI
   * container's auto-registration.
   *
   * The default is empty: a feature without dependencies.
   *
   * @return the names of the required features */
  virtual std::vector<std::string_view> requiredFeatures() const { return {}; }
};

/** Base template class for enum-based features that auto-register with the Features manager */
template<typename T, std::enable_if_t<std::is_enum_v<T>> * = nullptr> class Feature : public FeatureInterface
{
private:
  std::shared_ptr<Features> m_features;

protected:
  /** The name of this feature derived from the enum value.
   *
   * Stored by value on purpose: magic_enum::enum_name() returns a view into
   * static storage, but the initialiser below used to build a temporary
   * std::string and store a std::string_view of it. That view dangled as soon
   * as the constructor returned, so getName() handed out freed stack bytes -
   * which StartupBuilder::verifyFeatureDependencies() then compared against. */
  std::string m_name;

public:
  /** Construct a feature and register it with the central Features manager
   * @param type The enum value identifying this feature
   * @param features The central Features manager instance */
  Feature(T type, std::shared_ptr<Features> features)
    : m_name(magic_enum::enum_name(type)), m_features(std::move(features))
  {
    m_features->registerFeature(
      type,
      [&](lintel::di::ContainerBuilder &builder) { registerTypes(builder); },
      [&](std::shared_ptr<lintel::di::Container> container) { initialize(container); });
  }

  /** Virtual destructor */
  virtual ~Feature() = default;

  /** Register dependency injection types for this feature */
  virtual void registerTypes(lintel::di::ContainerBuilder &builder) = 0;

  /** Initialize the feature with the resolved container */
  virtual void initialize(std::shared_ptr<lintel::di::Container> container) = 0;

  /** Get the name of this feature
   * @return The feature name as a string view */
  std::string_view getName() { return m_name; }
};

}// namespace lintel
#endif// LINTEL_FEATURE_H
