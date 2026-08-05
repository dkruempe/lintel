#include "base_library/core/configuration/Configuration.h"

#include <tinyxml2.h>

#include <algorithm>
#include <utility>

#include "base_library/config.h"
#include "base_library/core/services/FileService.h"
#include "base_library/core/configuration/ConfigurationException.h"

Configuration::Configuration(
        const std::vector<std::shared_ptr<Component>> &components,
        std::shared_ptr<EnvironmentConfiguration> environmentConfiguration)
        : m_components(initialize(components)),
          m_environmentConfiguration(std::move(environmentConfiguration)),
          m_configurationFile(m_environmentConfiguration->of(EnvironmentConfiguration::ConfigDirectory) +
                              std::filesystem::path::preferred_separator +
                              m_environmentConfiguration->of(EnvironmentConfiguration::BootstrapConfigName) + ".xml") {
    loadConfiguration();
}

std::map<std::string, std::shared_ptr<Component>> Configuration::initialize(
        const std::vector<std::shared_ptr<Component>> &tempComponents) {
    std::map<std::string, std::shared_ptr<Component>> components;
    for (auto &component: tempComponents) {
        components.insert({component->getConfigRoot(), component});
    }
    return components;
}

void Configuration::loadConfiguration() {
    FileService file(m_configurationFile);
    if (!file.exists()) {
        return;
    }

    const std::string &content = file.readFile();
    tinyxml2::XMLDocument document;
    document.Parse(content.c_str());

    tinyxml2::XMLElement *rootElement =
            document.FirstChildElement("Configuration");
    if (rootElement == nullptr) {
        return;
    }

    for (tinyxml2::XMLElement *iter = rootElement->FirstChildElement();
         iter != nullptr; iter = iter->NextSiblingElement()) {
        std::string componentName(iter->Name());
        if (componentName.empty()) {
            throw ConfigurationException(
                    "Configuration", "no component name available", iter->GetLineNum());
        }

        int32_t lineOffset = iter->GetLineNum();
        tinyxml2::XMLPrinter printer;
        iter->Accept(&printer);

        auto found = m_components.find(componentName);
        if (found == m_components.end()) {
            throw ConfigurationException(
                    "Configuration", "no component name defined for " + componentName,
                    iter->GetLineNum());
        }

        auto entries =
                found->second->parse(printer.CStr(), file.getName(), lineOffset);
        for (auto &entry: entries) {
            m_configurationEntries.push_back(entry);
        }
    }
}

void Configuration::setEntries(std::vector<std::shared_ptr<Entry>> entries) {
    m_configurationEntries = std::move(entries);
}
