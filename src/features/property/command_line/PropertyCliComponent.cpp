#include "base_library/features/property/command_line/PropertyCliComponent.h"
#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include <tabulate/table.hpp>

#include "base_library/core/services/LoggerService.h"

PropertyCliComponent::PropertyCliComponent(
    std::shared_ptr<PropertyApi> propertyApi)
    : CommandLineComponent(m_name, m_alias),
      m_propertyApi(std::move(propertyApi)) {
  // Command: show all properties
  Command commandShowAllProperties("sap",
                                   "Shows all available valid properties!");
  m_commandParser.addCommand(commandShowAllProperties, AllProperties);
  // Command: show all properties of process
  Command commandShowAllPropertiesProcess(
      "sapp", "Shows all available valid properties for given process!");
  commandShowAllPropertiesProcess.addArgument(
      {"--process_name", "-p"}, &m_processName,
      "Process Name where property is defined!");
  m_commandParser.addCommand(commandShowAllPropertiesProcess,
                             ProcessProperties);
  // Command: show all properties of process and class
  Command commandShowAllPropertiesProcessClass(
      "sappc",
      "Shows all available valid properties for given process and class!");
  commandShowAllPropertiesProcessClass.addArgument(
      {"--process_name", "-p"}, &m_processName,
      "Process Name where property is defined!");
  commandShowAllPropertiesProcessClass.addArgument(
      {"--class_name", "-c"}, &m_className,
      "Class name where property is defined!");
  m_commandParser.addCommand(commandShowAllPropertiesProcessClass,
                             ProcessClassProperties);
  // Command: show all properties of process, class and instance
  Command commandShowAllPropertiesProcessClassInstance(
      "sappci",
      "Shows all available valid properties for given process, class and "
      "instance!");
  commandShowAllPropertiesProcessClassInstance.addArgument(
      {"--process_name", "-p"}, &m_processName,
      "Process Name where property is defined!");
  commandShowAllPropertiesProcessClassInstance.addArgument(
      {"--class_name", "-c"}, &m_className,
      "Class Name where property is defined!");
  commandShowAllPropertiesProcessClassInstance.addArgument(
      {"--instance_name", "-i"}, &m_instanceName,
      "Instance Name where property is defined!");
  m_commandParser.addCommand(commandShowAllPropertiesProcessClassInstance,
                             ProcessClassInstanceProperties);
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
        printProperties(m_propertyApi->allOf(m_processName));
        break;
      case ProcessClassProperties:
        printProperties(m_propertyApi->allOf(m_processName, m_className));
        break;
      case ProcessClassInstanceProperties:
        printProperties(
            m_propertyApi->allOf(m_processName, m_className, m_instanceName));
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
