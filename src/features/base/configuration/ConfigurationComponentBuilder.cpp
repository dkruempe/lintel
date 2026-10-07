#include "lintel/features/base/configuration/ConfigurationComponentBuilder.h"

#include <memory>

#include "lintel/features/base/configuration/DatabaseConnectionComponent.h"
#include "lintel/features/base/configuration/EventBusComponent.h"
#include "lintel/features/base/configuration/HistoryComponent.h"
#include "lintel/features/base/configuration/LoggerComponent.h"
#include "lintel/features/base/configuration/MessageQueueComponent.h"
#include "lintel/features/base/configuration/ProcessComponent.h"
#include "lintel/features/base/configuration/SharedMemorySegmentComponent.h"
#include "lintel/features/http/configuration/HttpComponent.h"
#include "lintel/features/property/configuration/PropertyComponent.h"
#include "lintel/features/property/configuration/PropertyRepositoryComponent.h"

ConfigurationComponentBuilder::ConfigurationComponentBuilder(
        const std::shared_ptr<EnvironmentConfiguration> &environmentConfiguration) {
    m_components.push_back(
            std::make_shared<DatabaseConnectionComponent>(environmentConfiguration));
    m_components.push_back(
            std::make_shared<HttpComponent>(environmentConfiguration));
    m_components.push_back(std::make_shared<PropertyComponent>());
    m_components.push_back(
            std::make_shared<LoggerComponent>(environmentConfiguration));
    m_components.push_back(
            std::make_shared<SharedMemorySegmentComponent>(environmentConfiguration));
    m_components.push_back(std::make_shared<PropertyRepositoryComponent>());
    m_components.push_back(std::make_shared<ProcessComponent>());
    m_components.push_back(std::make_shared<MessageQueueComponent>());
    m_components.push_back(std::make_shared<HistoryComponent>());
    m_components.push_back(std::make_shared<EventBusComponent>());
}

void ConfigurationComponentBuilder::add(
        std::shared_ptr<Component> &&component) {
    m_components.push_back(std::move(component));
}

void ConfigurationComponentBuilder::add(
        const std::shared_ptr<Component> &component) {
    m_components.push_back(component);
}

const std::vector<std::shared_ptr<Component>>
&ConfigurationComponentBuilder::build() {
    return m_components;
}