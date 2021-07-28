#ifndef CPP_BASE_LIBRARY_PROPERTYCLICOMPONENT_H
#define CPP_BASE_LIBRARY_PROPERTYCLICOMPONENT_H

#include <map>
#include <memory>
#include <string>

#include "base_library/features/cli/models/CommandLineComponent.h"
#include "base_library/features/property/controller/PropertyApi.h"

class PropertyCliComponent : public CommandLineComponent {
 private:
  enum Command {
    CommandUndefined,
    CommandAllProperties,
    CommandAllPropertiesOfProcess
  };
  std::map<std::string_view, Command> m_commands = {
      {"all_properties", CommandAllProperties},
      {"aprop", CommandAllProperties},
      {"all_properties_of_process", CommandAllPropertiesOfProcess},
      {"aprpr", CommandAllPropertiesOfProcess}};

  static constexpr std::string_view m_name = "Property";
  static constexpr std::string_view m_alias = "Prop";
  std::shared_ptr<PropertyApi> m_propertyApi;
  Command m_currentCommand = CommandUndefined;

  std::string m_process = "";
  std::string m_class = "";
  std::string m_instance = "";
  std::string m_propertyName = "";

  static void printProperties(const std::vector<PropertyDto> &properties);

 public:
  explicit PropertyCliComponent(std::shared_ptr<PropertyApi> propertyApi);

  void onCommand(const std::string &input, const std::vector<std::string> &parameters) override;

  void onHelp() override;

  void onShowMenu() override;

  bool onMenu(const std::string &component) override;

  bool onExit() override;
  void allOf();
};

#endif  // CPP_BASE_LIBRARY_PROPERTYCLICOMPONENT_H
