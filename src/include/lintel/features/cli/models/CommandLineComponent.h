#ifndef LINTEL_COMMANDLINECOMPONENT_H
#define LINTEL_COMMANDLINECOMPONENT_H

#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "lintel/features/http/service/Client.h"

class UserDto;

/** Abstract base class for all CLI components (menus/screens) */
class CommandLineComponent {
private:
    std::string_view m_name;
    std::string_view m_alias;

public:
    /** @param name display name of the component
     *  @param alias short alias for navigation */
    CommandLineComponent(std::string_view name, std::string_view alias);

    virtual ~CommandLineComponent() = default;

    /** @return component display name */
    std::string_view getName();

    /** @return component alias */
    std::string_view getAlias();

    /** Handle a command entered by the user */
    virtual void onCommand(const UserDto &userDto, const std::string &command,
                           const std::vector<std::string> &parameters) = 0;

    /** Show help for this component */
    virtual void onHelp() = 0;

    /** Show the menu for this component */
    virtual void onShowMenu() = 0;

    /** @param component name to navigate to; return true if handled */
    virtual bool onMenu(const std::string &component) = 0;

    /** @return true if exit was confirmed */
    virtual bool onExit() = 0;

    /** Print the list of available commands */
    virtual void printCommandList(std::set<std::string> menuAlias) = 0;

    /** @return set of sub-menu entries */
    virtual std::set<std::string> menuEntriesOf() { return {}; }

    /** @return vector of all command names */
    virtual std::vector<std::string> allCommandsOf() = 0;
};

#endif  // LINTEL_COMMANDLINECOMPONENT_H
