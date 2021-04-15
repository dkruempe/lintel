#include "base_library/configuration/Configuration.h"
#include "base_library/config.h"
#include "base_library/services/FileService.h"
#include "base_library/services/LoggerService.h"

#include <algorithm>
#include <tinyxml2.h>
#include <utility>

Configuration::Configuration(
    const std::vector<std::shared_ptr<Component>> &components,
    const std::string &configurationFileName)
    : components(initialize(components)),
      configurationFile(std::string(CONFIG_DIRECTORY) +
                        std::filesystem::path::preferred_separator +
                        configurationFileName + ".xml") {
  loadConfiguration();
}

std::map<std::string, std::shared_ptr<Component>> Configuration::initialize(
    const std::vector<std::shared_ptr<Component>> &tempComponents) {
  std::map<std::string, std::shared_ptr<Component>> components;
  for (auto &component : tempComponents) {
    components.insert({component->getConfigRoot(), component});
  }
  return components;
}

void Configuration::loadConfiguration() {
  FileService file(configurationFile);
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
    LOG_INFO("deserialize of {}", componentName);
    if (componentName.empty()) {
      LOG_ERROR("no component name avaialble at {}", iter->GetLineNum());
      continue;
    }

    int32_t lineOffset = iter->GetLineNum();
    tinyxml2::XMLPrinter printer;
    iter->Accept(&printer);

    auto found = components.find(componentName);
    if (found == components.end()) {
      LOG_ERROR("no component defined for {}", componentName);
      continue;
    }

    auto entries =
        found->second->parse(printer.CStr(), file.getName(), lineOffset);
    for (auto &iter : entries) {
      configurationEntries.push_back(iter);
    }
  }
}
Configuration::Configuration(std::vector<std::shared_ptr<Entry>> entries)
    : configurationEntries(std::move(entries)) {}
