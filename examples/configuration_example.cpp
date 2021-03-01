#include <ostream>
#include <tinyxml2.h>
#include <utility>

#include "base_library/configuration/Component.h"
#include "base_library/configuration/Configuration.h"
#include "base_library/configuration/PropertyComponent.h"
#include "base_library/configuration/PropertyEntry.h"
#include "base_library/services/LoggerService.h"
#include "base_library/utils/TypeName.h"

// Environment
#define ENVIRONMENT_ROOT "Environment"
#define ENVIRONMENT_NAME "name"
#define ENVIRONMENT_VALUE "value"

class EnvironmentConfigurationEntry : public Entry {
private:
  std::string name;
  std::string value;

public:
  EnvironmentConfigurationEntry(std::string_view configurationParserComponent,
                                std::string name, std::string value)
      : Entry(configurationParserComponent), name(std::move(name)),
        value(std::move(value)) {}

  [[nodiscard]] const std::string &getName() const { return name; }
  [[nodiscard]] const std::string &getValue() const { return value; }

  friend std::ostream &operator<<(std::ostream &os,
                                  const EnvironmentConfigurationEntry &entry) {
    os << " name: " << entry.name << " value: " << entry.value;
    return os;
  }
};

class EnvironmentConfigurationParserComponent : public Component {
public:
  EnvironmentConfigurationParserComponent() : Component("Environments") {}

  std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                            const std::string &fileName,
                                            const int32_t lineOffset) override {
    std::vector<std::shared_ptr<Entry>> environments;
    tinyxml2::XMLDocument document;
    document.Parse(content.c_str());

    tinyxml2::XMLElement *rootNode =
        document.FirstChildElement(getConfigRoot().c_str());
    if (rootNode == nullptr) {
      return environments;
    }

    for (tinyxml2::XMLElement *environmentElement =
             rootNode->FirstChildElement();
         environmentElement != nullptr;
         environmentElement = environmentElement->NextSiblingElement()) {
      if (std::strcmp(environmentElement->Name(), ENVIRONMENT_ROOT) != 0) {
        continue;
      }
      const char *name = environmentElement->Attribute(ENVIRONMENT_NAME);
      const char *value = environmentElement->Attribute(ENVIRONMENT_VALUE);
      if (name == nullptr) {
        LOG_ERROR("name of environemnt is null at {}",
                  environmentElement->GetLineNum());
        continue;
      }
      if (value == nullptr) {
        LOG_ERROR("value of environment is null at {}",
                  environmentElement->GetLineNum());
        continue;
      }
      environments.push_back(std::make_shared<EnvironmentConfigurationEntry>(
          type_name<EnvironmentConfigurationParserComponent>(), name, value));
    }
    return environments;
  }
};

int main(int argc, char *argv[]) {
  DECLARE_LOGGER(std::filesystem::path(argv[0]).filename());
  std::shared_ptr<Component> component = std::make_shared<PropertyComponent>();
  std::shared_ptr<EnvironmentConfigurationParserComponent> environment =
      std::make_shared<EnvironmentConfigurationParserComponent>();
  Configuration configurationParser({component, environment}, "bootstrap");
  std::vector<std::shared_ptr<Entry>> properties =
      configurationParser.configurationOf<PropertyComponent>();
  for (auto &iter : properties) {
    auto propertyPtr = std::static_pointer_cast<PropertyEntry>(iter);
    std::stringstream ss;
    ss << *propertyPtr;
    LOG_INFO("{}", ss.str());
  }
  std::vector<std::shared_ptr<Entry>> environments =
      configurationParser
          .configurationOf<EnvironmentConfigurationParserComponent>();
  for (auto &iter : environments) {
    auto environemntPtr =
        std::static_pointer_cast<EnvironmentConfigurationEntry>(iter);
    std::stringstream ss;
    ss << *environemntPtr;
    LOG_INFO("{}", ss.str());
  }
  return 0;
}