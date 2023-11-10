#ifndef CPP_BASE_LIBRARY_FEATURES_H
#define CPP_BASE_LIBRARY_FEATURES_H

#include <utility>
#include <vector>
#include <string>
#include <map>
#include <magic_enum.hpp>
#include "Hypodermic/ContainerBuilder.h"
#include "Hypodermic/Container.h"

/**
 * Features class for configuration support of Features, which are active
 *
 * Usage:
 *
 * - Define / Implement Feature
 * -
 */
class Features {
public:
    using RegisterTypeFunc = std::function<void(Hypodermic::ContainerBuilder &)>;
    using InitializeFunc = std::function<void(std::shared_ptr<Hypodermic::Container>)>;

    template<typename T, std::enable_if_t<std::is_enum_v<T>> * = nullptr>
    void registerFeature(T x, const RegisterTypeFunc &registerTypeFunc, const InitializeFunc &initializeFunc) {
        std::string name = std::string(magic_enum::enum_name(x));
        m_features.push_back(name);
        m_registerTypeMap.insert({name, registerTypeFunc});
        m_initializeMap.insert({name, initializeFunc});
    };

    Features() {
        auto names = magic_enum::enum_names<Value>();
        for (const auto &iter: names) {
            m_features.emplace_back(iter);
        }
    }

    void initializeOf(const std::string &featureName, std::shared_ptr<Hypodermic::Container> container) {
        auto found = m_initializeMap.find(featureName);
        if (found == m_initializeMap.end()) {
            return;
        }
        found->second(std::move(container));
    }

    void registerType(const std::string &featureName, Hypodermic::ContainerBuilder &builder) {
        auto found = m_registerTypeMap.find(featureName);
        if (found == m_registerTypeMap.end()) {
            return;
        }
        found->second(builder);
    }

    std::vector<std::string> allOf() {
        return m_features;
    }

    enum Value {
        Base,
        Http,
        Property,
        Cli
    };

private:
    std::vector<std::string> m_features{};
    std::map<std::string, RegisterTypeFunc> m_registerTypeMap;
    std::map<std::string, InitializeFunc> m_initializeMap;
};

#endif //CPP_BASE_LIBRARY_FEATURES_H
