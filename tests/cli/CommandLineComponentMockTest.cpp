#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include "../mocks/MockCommandLineComponent.h"

using namespace trompeloeil;

TEST_CASE("CommandLineComponent: getName and getAlias return constructor values") {
    MockCommandLineComponent component("TestName", "tn");
    REQUIRE(component.getName() == "TestName");
    REQUIRE(component.getAlias() == "tn");
}

TEST_CASE("CommandLineComponent: mock supports all virtual methods") {
    MockCommandLineComponent component("Test", "t");

    REQUIRE_CALL(component, onHelp());
    component.onHelp();

    REQUIRE_CALL(component, onShowMenu());
    component.onShowMenu();

    REQUIRE_CALL(component, onCommand(ANY(const UserDto &), ANY(const std::string &), ANY(const std::vector<std::string> &)));
    UserDto userDto;
    component.onCommand(userDto, "cmd", {});

    REQUIRE_CALL(component, onMenu("menu1"))
        .LR_RETURN(true);
    REQUIRE(component.onMenu("menu1") == true);

    REQUIRE_CALL(component, onExit())
        .LR_RETURN(true);
    REQUIRE(component.onExit() == true);

    REQUIRE_CALL(component, allCommandsOf())
        .LR_RETURN(std::vector<std::string>{"cmd1", "cmd2"});
    auto cmds = component.allCommandsOf();
    REQUIRE(cmds.size() == 2);

    REQUIRE_CALL(component, menuEntriesOf())
        .LR_RETURN(std::set<std::string>{"entry1"});
    auto entries = component.menuEntriesOf();
    REQUIRE(entries.size() == 1);
}
