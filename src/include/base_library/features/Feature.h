#ifndef CPP_BASE_LIBRARY_FEATURE_H
#define CPP_BASE_LIBRARY_FEATURE_H

#include <string>
#include <utility>

#include "base_library/core/utils/TypeName.h"
#include "Hypodermic/ContainerBuilder.h"
#include "Hypodermic/Container.h"
#include "base_library/features/Features.h"

/** Interface for all feature implementations */
class FeatureInterface {
public:
    /** Register dependency injection types for this feature */
    virtual void registerTypes(Hypodermic::ContainerBuilder &builder) = 0;

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
