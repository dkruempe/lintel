#pragma once

#include <memory>
#include <vector>

#include "base_library/core/configuration/Component.h"
#include "base_library/core/configuration/EnvironmentConfiguration.h"


/** Builder for assembling configuration parsing components */
class ConfigurationComponentBuilder {
private:
    /** The collection of components being built */
    std::vector<std::shared_ptr<Component>> m_components;

public:
    /** Construct a builder with the given environment configuration
     * @param environmentConfiguration The environment configuration */
    ConfigurationComponentBuilder(const std::shared_ptr<EnvironmentConfiguration>
                                  &environmentConfiguration);

    /** Destructor */
    ~ConfigurationComponentBuilder() = default;

    /** Add a component via move semantics
     * @param component The component to add */
    void add(std::shared_ptr<Component> &&component);

    /** Add a component via copy
     * @param component The component to add */
    void add(const std::shared_ptr<Component> &component);

    /** Build and return the component collection
     * @return Reference to the vector of components */
    [[nodiscard]] const std::vector<std::shared_ptr<Component>> &build();
};