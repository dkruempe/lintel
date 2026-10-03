#ifndef CPP_BASE_LIBRARY_FEATURE_H
#define CPP_BASE_LIBRARY_FEATURE_H

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "base_library/core/utils/TypeName.h"
#include "base_library/features/Features.h"

class Configuration;
class ProcessName;

// The DI container is only referenced through pointers/references in the
// signatures below, so an incomplete type is enough. Implementations of
// registerTypes()/initialize() include "Hypodermic/ContainerBuilder.h" and
// "Hypodermic/Container.h" themselves.
namespace Hypodermic {
class Container;
class ContainerBuilder;
}  // namespace Hypodermic

/** Interface for all feature implementations */
class FeatureInterface {
public:
    /** Register dependency injection types for this feature */
    virtual void registerTypes(Hypodermic::ContainerBuilder &builder) = 0;

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
    virtual void registerTypes(
            Hypodermic::ContainerBuilder &builder,
            const std::shared_ptr<Configuration> &configuration,
            const std::shared_ptr<ProcessName> &processName) {
        (void)configuration;
        (void)processName;
        registerTypes(builder);
    }

    /** Initialize the feature with the resolved container */
    virtual void initialize(std::shared_ptr<Hypodermic::Container> container) = 0;

    /** Get the name of this feature */
    virtual std::string_view getName() = 0;
};

/** Base template class for enum-based features that auto-register with the Features manager */
template<typename T, std::enable_if_t<std::is_enum_v<T>> * = nullptr>
class Feature : public FeatureInterface {
private:
    std::shared_ptr<Features> m_features;
protected:
    /** The name of this feature derived from the enum value */
    std::string_view m_name;

public:
    /** Construct a feature and register it with the central Features manager
     * @param type The enum value identifying this feature
     * @param features The central Features manager instance */
    Feature(T type, std::shared_ptr<Features> features) : m_name(std::string(magic_enum::enum_name(type))),
                                                          m_features(std::move(features)) {
        m_features->registerFeature(type, [&](Hypodermic::ContainerBuilder &builder) { registerTypes(builder); },
                                    [&](std::shared_ptr<Hypodermic::Container> container) { initialize(container); });
    }

    /** Virtual destructor */
    virtual ~Feature() = default;

    /** Register dependency injection types for this feature */
    virtual void registerTypes(Hypodermic::ContainerBuilder &builder) = 0;

    /** Initialize the feature with the resolved container */
    virtual void initialize(std::shared_ptr<Hypodermic::Container> container) = 0;

    /** Get the name of this feature
     * @return The feature name as a string view */
    std::string_view getName() { return m_name; }
};

#endif  // CPP_BASE_LIBRARY_FEATURE_H
