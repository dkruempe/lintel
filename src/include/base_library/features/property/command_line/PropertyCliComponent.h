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
    ProcessClassProperties
  };
  static constexpr std::string_view showAllProperties = "sap";
  static constexpr std::string_view showAllPropertiesProcess = "sapp";
  static constexpr std::string_view showAllPropertiesProcessClass = "sappc";
  static constexpr std::string_view showAllPropertiesProcessClassInstance =
      "sappci";
  static constexpr std::string_view showProperty = "sp";
  static constexpr std::string_view updateProperty = "up";

  // Flags
  std::string m_processNameValue;
  static constexpr std::string_view m_processName = "--process_name";
  static constexpr std::string_view m_processNameShort = "-p";

  std::string m_classNameValue;
  static constexpr std::string_view m_className = "--class_name";
  static constexpr std::string_view m_classNameShort = "-c";

  std::string m_instanceNameValue;
  static constexpr std::string_view m_instanceName = "--instance_name";
  static constexpr std::string_view m_instanceNameShort = "-i";

  std::string m_propertyNameValue;
  static constexpr std::string_view m_propertyName = "--property_name";
  static constexpr std::string_view m_propertyNameShort = "-p";

  std::string m_typeValue;
  static constexpr std::string_view m_type = "--type";
  static constexpr std::string_view m_typeShort = "-t";

  std::string m_valueValue;
  static constexpr std::string_view m_value = "--value";
  static constexpr std::string_view m_valueShort = "-v";

  CommandParser<Commands, Undefined> m_commandParser;

  static void printProperties(const std::vector<PropertyDto> &properties);

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
