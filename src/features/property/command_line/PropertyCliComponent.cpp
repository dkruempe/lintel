#include "base_library/features/property/command_line/PropertyCliComponent.h"

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/utils/TableBuilder.h"

PropertyCliComponent::PropertyCliComponent(
        std::shared_ptr<PropertyApi> propertyApi)
        : CommandLineComponent(m_name, m_alias),
          m_propertyApi(std::move(propertyApi)) {
    // Command: show all properties
    m_commandParser.addCommand(
            Command("show_properties", "Shows all available valid properties!")
                    .addArgument({"--process-name", "-p"}, &m_processName,
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
    TableBuilder<7> builder;
    builder.add({"No.", "Process", "Class", "Instance", "Name", "Type", "Value"});
    std::size_t iter = 0;
    for (const auto &property: properties) {
        builder.add({std::to_string(++iter), property.getProcessName(),
                     property.getClassName(), property.getInstanceName(),
                     property.getName(), property.getType(), property.getValue()});
    }
    std::cout << builder.build() << "\n";
}

void PropertyCliComponent::onHelp() {
    m_commandParser.printHelp(getName(), getAlias(), m_description);
}

bool PropertyCliComponent::onMenu(const std::string &component) { return true; }

void PropertyCliComponent::onShowMenu() {
    std::cout << "No submenu available!\n";
}

bool PropertyCliComponent::onExit() { return true; }

void PropertyCliComponent::printProperty(
        const std::optional<PropertyDto> &optionalProperty) {
    if (!optionalProperty.has_value()) {
        std::cerr << "Property with given values is not preset => No Update "
                     "possible \n";
        return;
    }
    const PropertyDto &propertyDto = optionalProperty.value();
    TableBuilder<2> builder;
    builder.add({"Process:", propertyDto.getProcessName()});
    builder.add({"Class:", propertyDto.getClassName()});
    builder.add({"Instance:", propertyDto.getInstanceName()});
    builder.add({"Name:", propertyDto.getName()});
    builder.add({"Type:", propertyDto.getType()});
    builder.add({"Value:", propertyDto.getValue()});
    builder.add({"Data Storage:", propertyDto.getRepositoryType().toString()});
    std::cout << builder.build() << "\n";
}

void PropertyCliComponent::printCommandList(std::set<std::string> menuAlias) {
    m_commandParser.printCommandList(menuAlias);
}

std::vector<std::string> PropertyCliComponent::allCommandsOf() {
    return m_commandParser.allCommandsOf();
}
