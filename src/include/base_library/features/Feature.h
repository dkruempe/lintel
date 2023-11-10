#ifndef CPP_BASE_LIBRARY_FEATURE_H
#define CPP_BASE_LIBRARY_FEATURE_H

#include <string>
#include <utility>

#include "base_library/core/utils/TypeName.h"
#include "Hypodermic/ContainerBuilder.h"
#include "Hypodermic/Container.h"
#include "base_library/features/Features.h"

class FeatureInterface {
public:
    virtual void registerTypes(Hypodermic::ContainerBuilder &builder) = 0;

    virtual void initialize(std::shared_ptr<Hypodermic::Container> container) = 0;

    virtual std::string_view getName() = 0;
};

template<typename T, std::enable_if_t<std::is_enum_v<T>> * = nullptr>
class Feature : public FeatureInterface {
private:
    std::shared_ptr<Features> m_features;
protected:
    std::string_view m_name;

public:
    Feature(T type, std::shared_ptr<Features> features) : m_name(std::string(magic_enum::enum_name(type))), m_features(std::move(features)) {
        m_features->registerFeature(type, [&](Hypodermic::ContainerBuilder &builder) { registerTypes(builder); },
                                   [&](std::shared_ptr<Hypodermic::Container> container) { initialize(container); });
    }

    virtual ~Feature() = default;

    virtual void registerTypes(Hypodermic::ContainerBuilder &builder) = 0;

    virtual void initialize(std::shared_ptr<Hypodermic::Container> container) = 0;

    std::string_view getName() { return m_name; }
};

#endif  // CPP_BASE_LIBRARY_FEATURE_H
