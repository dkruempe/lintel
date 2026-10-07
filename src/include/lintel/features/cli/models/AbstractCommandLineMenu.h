#ifndef LINTEL_ABSTRACTCOMMANDLINEMENU_H
#define LINTEL_ABSTRACTCOMMANDLINEMENU_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "lintel/features/cli/models/CommandLineComponent.h"

/** Manages a collection of CommandLineComponents and routes commands/menu navigation between them */
class AbstractCommandLineMenu {
private:
    std::vector<std::shared_ptr<CommandLineComponent>> m_components;
    std::map<std::string_view, std::shared_ptr<CommandLineComponent>>
            m_componentMap;
    std::shared_ptr<CommandLineComponent> m_current = nullptr;

    /** Build a name-to-component map from the component vector */
    static std::map<std::string_view, std::shared_ptr<CommandLineComponent>>
    build(const std::vector<std::shared_ptr<CommandLineComponent>> &components);

public:
    /** @param components the list of available CLI components */
    explicit AbstractCommandLineMenu(
            const std::vector<std::shared_ptr<CommandLineComponent>> &components);

    /** Show the main menu with all available components */
    void onShowMenu();

    /** @return set of all menu entry names */
    std::set<std::string> allMenuEntriesOf();

    /** @param command component name to navigate to */
    bool onMenu(const std::string &command);

    /** @return true if exit was confirmed */
    bool onExit();

    /** Print the command list for the current component */
    void printCommandList();

    /** Route command to the currently active component */
    void onCommand(const UserDto &userDto, const std::string &command,
                   const std::vector<std::string> &parameters);

    /** @return list of all commands across all components */
    std::vector<std::string> allCommandsOf();

    /** Show help for the current component */
    void onHelp();

    /** @return reference to the currently active component */
    const std::shared_ptr<CommandLineComponent> &currentOf();
};

#endif  // LINTEL_ABSTRACTCOMMANDLINEMENU_H
