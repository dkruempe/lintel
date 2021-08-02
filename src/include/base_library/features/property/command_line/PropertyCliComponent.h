#ifndef CPP_BASE_LIBRARY_PROPERTYCLICOMPONENT_H
#define CPP_BASE_LIBRARY_PROPERTYCLICOMPONENT_H

#include <map>
#include <memory>
#include <string>

#include "base_library/features/cli/models/CommandLineComponent.h"
#include "base_library/features/cli/models/CommandParser.h"
#include "base_library/features/property/controller/PropertyApi.h"

class PropertyCliComponent : public CommandLineComponent {
 private:
  static constexpr std::string_view m_name = "Property";
  static constexpr std::string_view m_alias = "Prop";
  std::shared_ptr<PropertyApi> m_propertyApi;

  // Commands
  enum Commands {
    Undefined,
    AllProperties,
    ProcessProperties,
    ProcessClassProperties,
    ProcessClassInstanceProperties,
    ShowProperty,
    UpdateProperty
  };

  // Flags
  std::string m_processName;
  std::string m_className;
  std::string m_instanceName;
  std::string m_propertyName;
  std::string m_value;

  CommandParser<Commands, Undefined> m_commandParser;

  static void printProperties(const std::vector<PropertyDto> &properties);

  static void printProperty(const std::optional<PropertyDto> &optionalProperty);

 public:
  explicit PropertyCliComponent(std::shared_ptr<PropertyApi> propertyApi);

  void onCommand(const std::string &input,
                 const std::vector<std::string> &parameters) override;

  void onHelp() override;

  void onShowMenu() override;

  bool onMenu(const std::string &component) override;

  bool onExit() override;
};

#endif  // CPP_BASE_LIBRARY_PROPERTYCLICOMPONENT_H
