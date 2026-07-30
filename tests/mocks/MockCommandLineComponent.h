#ifndef CPP_BASE_LIBRARY_MOCKCOMMANDLINECOMPONENT_H
#define CPP_BASE_LIBRARY_MOCKCOMMANDLINECOMPONENT_H

#include <catch2/trompeloeil.hpp>

#include "base_library/features/base/controller/UserDto.h"
#include "base_library/features/cli/models/CommandLineComponent.h"

class MockCommandLineComponent : public CommandLineComponent {
public:
    using CommandLineComponent::CommandLineComponent;

    MAKE_MOCK3(onCommand, void(const UserDto &, const std::string &, const std::vector<std::string> &), override);
    MAKE_MOCK0(onHelp, void(), override);
    MAKE_MOCK0(onShowMenu, void(), override);
    MAKE_MOCK1(onMenu, bool(const std::string &), override);
    MAKE_MOCK0(onExit, bool(), override);
    MAKE_MOCK1(printCommandList, void(std::set<std::string>), override);
    MAKE_MOCK0(allCommandsOf, std::vector<std::string>(), override);
    MAKE_MOCK0(menuEntriesOf, std::set<std::string>(), override);
};

#endif  // CPP_BASE_LIBRARY_MOCKCOMMANDLINECOMPONENT_H
