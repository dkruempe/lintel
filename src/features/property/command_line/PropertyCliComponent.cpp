#include "base_library/features/property/command_line/PropertyCliComponent.h"

#include <tabulate/table.hpp>

#include "base_library/core/services/LoggerService.h"

PropertyCliComponent::PropertyCliComponent(
    std::shared_ptr<PropertyApi> propertyApi)
    : CommandLineComponent(m_name, m_alias),
      m_propertyApi(std::move(propertyApi)) {
  // Command: show all properties
  m_commandParser.addCommand(
      Command("show_properties", "Shows all available valid properties!")
          .addArgument({"--proocess-name", "-p"}, &m_processName,
                       "Process Name of property")
          .addArgument({"--class-name", "-c"}, &m_className,
                       "Class Name of property")
          .addArgument({"--instance-name", "-i"}, &m_instanceName,
                       "Instance Name of property"),
      ShowProperties);
  // Command: show property
  m_commandParser.addCommand(
      Command("show_property", "Show given property")
          .addArgument({"--process_name", "-p"}, &m_processName,
                       "Process Name of property!")
          .addArgument({"--class_name", "-c"}, &m_className,
                       "Class Name of property!")
          .addArgument({"--instance-name", "-i"}, &m_instanceName,
                       "Instance Name of property")
          .addArgument({"--name", "-n"}, &m_propertyName, "Name of property!"),
      ShowProperty);
  // Command: update property
  m_commandParser.addCommand(
      Command("update_property", "Update given property")
          .addArgument({"--process_name", "-p"}, &m_processName,
                       "Process Name of property!")
          .addArgument({"--class_name", "-c"}, &m_className,
                       "Class Name of property!")
          .addArgument({"--instance-name", "-i"}, &m_instanceName,
                       "Instance Name of property")
          .addArgument({"--value", "-v"}, &m_value, "Value of property!")
          .addArgument({"--name", "-n"}, &m_propertyName, "Name of property!"),
      UpdateProperty);
}
void PropertyCliComponent::onCommand(
    const UserDto &userDto, const std::string &input,
    const std::vector<std::string> &parameters) {
  try {
    Commands command = m_commandParser.parse(input, parameters);
    switch (command) {
      case ShowProperties: {
        std::string processNameArg =
            m_processName.has_value() ? m_processName.value() : ".*";
        std::string classNameArg =
            m_className.has_value() ? m_className.value() : ".*";
        std::string instanceNameArg =
            m_instanceName.has_value() ? m_instanceName.value() : ".*";
        printProperties(m_propertyApi->allOf(processNameArg, classNameArg,
                                             instanceNameArg));
        break;
      }
      case ShowProperty: {
        auto optProperty =
            m_propertyApi->of(m_processName.value(), m_className.value(),
                              m_instanceName.value(), m_propertyName.value());
        printProperty(optProperty);
        break;
      }
      case UpdateProperty: {
        auto optProperty =
            m_propertyApi->of(m_processName.value(), m_className.value(),
                              m_instanceName.value(), m_propertyName.value());
        if (!optProperty.has_value()) {
          std::cerr << "Property with given values is not preset => No Update "
                       "possible \n";
          return;
        }
        std::cout << "Before Update: \n";
        printProperty(optProperty);
        m_propertyApi->updateOf(optProperty.value(), m_value.value());
        optProperty =
            m_propertyApi->of(m_processName.value(), m_className.value(),
                              m_instanceName.value(), m_propertyName.value());
        std::cout << "After Update: \n";
        printProperty(optProperty);
        break;
      }
      default:
        break;
    }
  } catch (const std::exception &exception) {
    std::cerr << "ERROR: " << exception.what() << "\n";
  }
}
void PropertyCliComponent::printProperties(
    const std::vector<PropertyDto> &properties) {
  tabulate::Table table;
  table.add_row(
      {"No.", "Process", "Class", "Instance", "Name", "Type", "Value"});
  std::size_t iter = 0;
  for (const auto &property : properties) {
    table.add_row({std::to_string(++iter), property.getProcessName(),
                   property.getClassName(), property.getInstanceName(),
                   property.getName(), property.getType(),
                   property.getValue()});
  }
  std::cout << table.str() << "\n";
}

void PropertyCliComponent::onHelp() {
  m_commandParser.printHelp(getName(), getAlias(), m_description);
}

bool PropertyCliComponent::onMenu(const std::string &component) { return true; }

void PropertyCliComponent::onShowMenu() {
  std::cout << "No submenu available!";
}

bool PropertyCliComponent::onExit() { return true; }
void PropertyCliComponent::printProperty(
    const optional<PropertyDto> &optionalProperty) {
  if (!optionalProperty.has_value()) {
    std::cerr << "Property with given values is not preset => No Update "
                 "possible \n";
    return;
  }
  const PropertyDto &propertyDto = optionalProperty.value();
  tabulate::Table table;
  table.add_row({"Process:", propertyDto.getProcessName()});
  table.add_row({"Class:", propertyDto.getClassName()});
  table.add_row({"Instance:", propertyDto.getInstanceName()});
  table.add_row({"Name:", propertyDto.getName()});
  table.add_row({"Type:", propertyDto.getType()});
  table.add_row({"Value:", propertyDto.getValue()});
  table.add_row({"Data Storage:", propertyDto.getRepositoryType().toString()});
  std::cout << table.str() << "\n";
}
void PropertyCliComponent::printCommandList() {
  m_commandParser.printCommandList();
}
