#ifndef CPP_BASE_LIBRARY_PROPERTYCLICOMPONENT_H
#define CPP_BASE_LIBRARY_PROPERTYCLICOMPONENT_H

#include <map>
#include <memory>
#include <string>

#include "base_library/features/cli/models/CommandLineComponent.h"
#include "base_library/features/cli/models/CommandParser.h"
#include "base_library/features/property/controller/PropertyApi.h"

/** CLI component for viewing and manipulating properties */
class PropertyCliComponent : public CommandLineComponent {
private:
    static constexpr std::string_view m_name = "Property";
    static constexpr std::string_view m_alias = "Prop";
    static constexpr std::string_view m_description =
            "The component can be used to show and manipulate all kind of available "
            "pmroperties.";
    std::shared_ptr<PropertyApi> m_propertyApi;

    // Commands
    enum Commands {
        Undefined, ShowProperties, ShowProperty, UpdateProperty
    };

    // Flags
    std::optional<std::string> m_processName;
    std::optional<std::string> m_className;
    std::optional<std::string> m_instanceName;
    std::optional<std::string> m_propertyName;
    std::optional<std::string> m_value;

    CommandParser<Commands, Undefined> m_commandParser;

    /** Print formatted property list to console */
    static void printProperties(const std::vector<PropertyDto> &properties);

    /** Print a single property to console */
    static void printProperty(const std::optional<PropertyDto> &optionalProperty);

public:
    /** @param propertyApi API for property operations */
    explicit PropertyCliComponent(std::shared_ptr<PropertyApi> propertyApi);

    void onCommand(const UserDto &userDto, const std::string &input,
                   const std::vector<std::string> &parameters) override;

    void onHelp() override;

    void onShowMenu() override;

    bool onMenu(const std::string &component) override;

    bool onExit() override;

    void printCommandList(std::set<std::string> menuAlias) override;

    std::vector<std::string> allCommandsOf() override;
};

#endif  // CPP_BASE_LIBRARY_PROPERTYCLICOMPONENT_H
