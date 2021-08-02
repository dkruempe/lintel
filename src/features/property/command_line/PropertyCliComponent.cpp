#include "base_library/features/property/command_line/PropertyCliComponent.h"
#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include <tabulate/table.hpp>

#include "base_library/core/services/LoggerService.h"

PropertyCliComponent::PropertyCliComponent(
    std::shared_ptr<PropertyApi> propertyApi)
    : CommandLineComponent(m_name, m_alias),
      m_propertyApi(std::move(propertyApi)) {
  Command commandShowAllProperties(showAllProperties,
                                   "Shows all available valid properties!");
  m_commandParser.addCommand(commandShowAllProperties, AllProperties);
  Command commandShowAllPropertiesProcess(
      showAllPropertiesProcess,
      "Shows all available valid properties for given process!");
  commandShowAllPropertiesProcess.addArgument(
      {std::string(m_processName), std::string(m_processNameShort)},
      &m_processNameValue, "Process Name where property is defined!");
  m_commandParser.addCommand(commandShowAllPropertiesProcess,
                             ProcessProperties);
  Command commandShowAllPropertiesProcessClass(
      showAllPropertiesProcessClass,
      "Shows all available valid properties for given process and class!");
  commandShowAllPropertiesProcessClass.addArgument(
      {std::string(m_processName), std::string(m_processNameShort)},
      &m_processNameValue, "Process Name where property is defined!");
  commandShowAllPropertiesProcessClass.addArgument(
      {std::string(m_className), std::string(m_classNameShort)},
      &m_classNameValue, "Class name where property is defined!");
  m_commandParser.addCommand(commandShowAllPropertiesProcessClass,
                             ProcessClassProperties);
}
void PropertyCliComponent::onCommand(
    const std::string &input, const std::vector<std::string> &parameters) {
  try {
    Commands command = m_commandParser.parse(input, parameters);
    switch (command) {
      case AllProperties:
        printProperties(m_propertyApi->allOf());
        break;
      case ProcessProperties:
        printProperties(m_propertyApi->allOf(m_processNameValue));
        break;
      case ProcessClassProperties:
        printProperties(
            m_propertyApi->allOf(m_processNameValue, m_classNameValue));
        break;
      default:
        break;
    }
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << "\n";
  }
}
void PropertyCliComponent::printProperties(
    const std::vector<PropertyDto> &properties) {
  tabulate::Table table;
  table.add_row({"No.", "Process", "Class", "Instance", "Name", "Type", "Value",
                 "Data Storage"});
  std::size_t iter = 0;
  for (const auto &property : properties) {
    table.add_row({std::to_string(++iter), property.getProcessName(),
                   property.getClassName(), property.getInstanceName(),
                   property.getName(), property.getType(), property.getValue(),
                   property.getRepositoryType().toString()});
  }
  fmt::print("{}\n", table.str());
}

void PropertyCliComponent::onHelp() { m_commandParser.printHelp(); }

bool PropertyCliComponent::onMenu(const std::string &component) { return true; }

void PropertyCliComponent::onShowMenu() { fmt::print("No submenu available!"); }

bool PropertyCliComponent::onExit() { return true; }
