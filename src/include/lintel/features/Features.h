#ifndef LINTEL_FEATURES_H
#define LINTEL_FEATURES_H

#include <functional>
#include <utility>
#include <vector>
#include <string>
#include <map>
#include <magic_enum/magic_enum.hpp>

// Hypodermic is only needed as an incomplete type here: ContainerBuilder is
// passed by reference and Container travels inside a shared_ptr. Forward
// declaring both keeps the DI container - and the Boost headers it used to
// pull in - out of every translation unit that only talks to Features.
namespace Hypodermic {
class Container;
class ContainerBuilder;
}  // namespace Hypodermic

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
    /** Function type for registering types with the DI container */
    using RegisterTypeFunc = std::function<void(Hypodermic::ContainerBuilder &)>;
    /** Function type for initializing a feature with the resolved container */
    using InitializeFunc = std::function<void(std::shared_ptr<Hypodermic::Container>)>;

    /** Register a feature identified by its enum value
     * @tparam T The enum type
     * @param x The enum value identifying the feature
     * @param registerTypeFunc Callback to register DI types
     * @param initializeFunc Callback to initialize the feature */
    template<typename T, std::enable_if_t<std::is_enum_v<T>> * = nullptr>
    void registerFeature(T x, const RegisterTypeFunc &registerTypeFunc, const InitializeFunc &initializeFunc) {
        std::string name = std::string(magic_enum::enum_name(x));
        m_features.push_back(name);
        m_registerTypeMap.insert({name, registerTypeFunc});
        m_initializeMap.insert({name, initializeFunc});
    };

    /** Initialize the Features manager with all known enum values */
    Features() {
        auto names = magic_enum::enum_names<Value>();
        for (const auto &iter: names) {
            m_features.emplace_back(iter);
        }
    }

    /** Initialize a specific feature by name
     * @param featureName The name of the feature to initialize
     * @param container The resolved DI container */
    void initializeOf(const std::string &featureName, std::shared_ptr<Hypodermic::Container> container) {
        auto found = m_initializeMap.find(featureName);
        if (found == m_initializeMap.end()) {
            return;
        }
        found->second(std::move(container));
    }

    /** Register types for a specific feature by name
     * @param featureName The name of the feature
     * @param builder The DI container builder */
    void registerType(const std::string &featureName, Hypodermic::ContainerBuilder &builder) {
        auto found = m_registerTypeMap.find(featureName);
        if (found == m_registerTypeMap.end()) {
            return;
        }
        found->second(builder);
    }

    /** Get all registered feature names
     * @return Vector of all feature names */
    std::vector<std::string> allOf() {
        return m_features;
    }

    /** Enumeration of known features */
    enum Value {
        Base,    /**< Base library feature */
        Http,    /**< HTTP feature */
        Property, /**< Property feature */
        Cli      /**< CLI feature */
    };

private:
    /** List of registered feature names */
    std::vector<std::string> m_features{};
    /** Map of feature names to their type registration callbacks */
    std::map<std::string, RegisterTypeFunc> m_registerTypeMap;
    /** Map of feature names to their initialization callbacks */
    std::map<std::string, InitializeFunc> m_initializeMap;
};

#endif //LINTEL_FEATURES_H
