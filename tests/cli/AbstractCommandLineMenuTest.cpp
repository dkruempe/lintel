#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include <algorithm>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../mocks/MockCommandLineComponent.h"
#include "lintel/features/base/controller/UserDto.h"
#include "lintel/features/cli/models/AbstractCommandLineMenu.h"

namespace lintel {
using namespace trompeloeil;

namespace {

  /** RAII redirector for `std::cout`, restored on destruction even when a REQUIRE throws. */
  class acmCoutCapture
  {
  public:
    acmCoutCapture() : m_oldBuffer(std::cout.rdbuf(m_stream.rdbuf())) {}

    ~acmCoutCapture() { std::cout.rdbuf(m_oldBuffer); }

    acmCoutCapture(const acmCoutCapture &) = delete;
    acmCoutCapture &operator=(const acmCoutCapture &) = delete;

    [[nodiscard]] std::string str() const { return m_stream.str(); }

  private:
    std::ostringstream m_stream;
    std::streambuf *m_oldBuffer;
  };

  std::shared_ptr<MockCommandLineComponent> acmComponent(const std::string &name, const std::string &alias)
  {
    return std::make_shared<MockCommandLineComponent>(name, alias);
  }

}// namespace

TEST_CASE("AbstractCommandLineMenu: currentOf is empty initially")
{
  auto component = acmComponent("Alpha", "Alp");
  AbstractCommandLineMenu menu({ component });

  REQUIRE(menu.currentOf() == nullptr);
}

TEST_CASE("AbstractCommandLineMenu: onShowMenu prints every component")
{
  auto alpha = acmComponent("Alpha", "Alp");
  auto beta = acmComponent("Beta", "Bet");
  AbstractCommandLineMenu menu({ alpha, beta });

  acmCoutCapture out;
  menu.onShowMenu();
  const std::string printed = out.str();

  REQUIRE(printed.find("Menu Name") != std::string::npos);
  REQUIRE(printed.find("Alpha") != std::string::npos);
  REQUIRE(printed.find("Alp") != std::string::npos);
  REQUIRE(printed.find("Beta") != std::string::npos);
}

TEST_CASE("AbstractCommandLineMenu: onShowMenu delegates to the active component")
{
  auto alpha = acmComponent("Alpha", "Alp");
  AbstractCommandLineMenu menu({ alpha });

  REQUIRE(menu.onMenu("Alpha"));
  REQUIRE_CALL(*alpha, onShowMenu()).TIMES(1);

  acmCoutCapture out;
  menu.onShowMenu();
  REQUIRE(out.str().empty());
}

TEST_CASE("AbstractCommandLineMenu: onMenu resolves by name")
{
  auto alpha = acmComponent("Alpha", "Alp");
  AbstractCommandLineMenu menu({ alpha });

  REQUIRE(menu.onMenu("Alpha"));
  REQUIRE(menu.currentOf() == alpha);
}

TEST_CASE("AbstractCommandLineMenu: onMenu resolves by alias")
{
  auto alpha = acmComponent("Alpha", "Alp");
  AbstractCommandLineMenu menu({ alpha });

  REQUIRE(menu.onMenu("Alp"));
  REQUIRE(menu.currentOf() == alpha);
}

TEST_CASE("AbstractCommandLineMenu: onMenu returns false for an unknown component")
{
  auto alpha = acmComponent("Alpha", "Alp");
  AbstractCommandLineMenu menu({ alpha });

  REQUIRE_FALSE(menu.onMenu("DoesNotExist"));
  REQUIRE(menu.currentOf() == nullptr);
}

TEST_CASE("AbstractCommandLineMenu: onMenu delegates to the active component once entered")
{
  auto alpha = acmComponent("Alpha", "Alp");
  AbstractCommandLineMenu menu({ alpha });

  REQUIRE(menu.onMenu("Alp"));
  REQUIRE(menu.currentOf() == alpha);

  REQUIRE_CALL(*alpha, onMenu("sub")).TIMES(1).LR_RETURN(true);
  REQUIRE(menu.onMenu("sub"));
}

TEST_CASE("AbstractCommandLineMenu: onExit delegates and clears the active component")
{
  auto alpha = acmComponent("Alpha", "Alp");
  AbstractCommandLineMenu menu({ alpha });

  REQUIRE(menu.onMenu("Alpha"));
  REQUIRE_CALL(*alpha, onExit()).TIMES(1).LR_RETURN(true);

  REQUIRE(menu.onExit());
  REQUIRE(menu.currentOf() == nullptr);
}

TEST_CASE("AbstractCommandLineMenu: onCommand and onHelp are no-ops without an active component")
{
  auto alpha = acmComponent("Alpha", "Alp");
  AbstractCommandLineMenu menu({ alpha });

  FORBID_CALL(*alpha, onCommand(ANY(const UserDto &), ANY(const std::string &), ANY(const std::vector<std::string> &)));
  FORBID_CALL(*alpha, onHelp());

  menu.onCommand(UserDto{}, "whatever", {});
  menu.onHelp();
  SUCCEED();
}

TEST_CASE("AbstractCommandLineMenu: onCommand and onHelp delegate to the active component")
{
  auto alpha = acmComponent("Alpha", "Alp");
  AbstractCommandLineMenu menu({ alpha });

  REQUIRE(menu.onMenu("Alpha"));

  REQUIRE_CALL(*alpha, onCommand(ANY(const UserDto &), "show", ANY(const std::vector<std::string> &))).TIMES(1);
  REQUIRE_CALL(*alpha, onHelp()).TIMES(1);

  menu.onCommand(UserDto{}, "show", { "--flag" });
  menu.onHelp();
}

TEST_CASE("AbstractCommandLineMenu: allCommandsOf lists component names at the root")
{
  auto alpha = acmComponent("Alpha", "Alp");
  auto beta = acmComponent("Beta", "Bet");
  AbstractCommandLineMenu menu({ alpha, beta });

  const std::vector<std::string> commands = menu.allCommandsOf();
  REQUIRE(std::find(commands.begin(), commands.end(), "Alpha") != commands.end());
  REQUIRE(std::find(commands.begin(), commands.end(), "Beta") != commands.end());
}

TEST_CASE("AbstractCommandLineMenu: allCommandsOf appends the active component commands and exit")
{
  auto alpha = acmComponent("Alpha", "Alp");
  AbstractCommandLineMenu menu({ alpha });

  REQUIRE(menu.onMenu("Alpha"));
  ALLOW_CALL(*alpha, allCommandsOf()).LR_RETURN(std::vector<std::string>{ "show", "update" });

  const std::vector<std::string> commands = menu.allCommandsOf();
  REQUIRE(std::find(commands.begin(), commands.end(), "show") != commands.end());
  REQUIRE(std::find(commands.begin(), commands.end(), "update") != commands.end());
  REQUIRE(std::find(commands.begin(), commands.end(), "exit") != commands.end());
}

TEST_CASE("AbstractCommandLineMenu: allMenuEntriesOf lists component names at the root")
{
  auto alpha = acmComponent("Alpha", "Alp");
  auto beta = acmComponent("Beta", "Bet");
  AbstractCommandLineMenu menu({ alpha, beta });

  const std::set<std::string> entries = menu.allMenuEntriesOf();
  REQUIRE(entries.count("Alpha") == 1);
  REQUIRE(entries.count("Beta") == 1);
}

TEST_CASE("AbstractCommandLineMenu: allMenuEntriesOf delegates to the active component")
{
  auto alpha = acmComponent("Alpha", "Alp");
  AbstractCommandLineMenu menu({ alpha });

  REQUIRE(menu.onMenu("Alpha"));
  ALLOW_CALL(*alpha, menuEntriesOf()).LR_RETURN(std::set<std::string>{ "sub" });

  REQUIRE(menu.allMenuEntriesOf() == std::set<std::string>{ "sub" });
}

TEST_CASE("AbstractCommandLineMenu: printCommandList is a no-op without an active component")
{
  auto alpha = acmComponent("Alpha", "Alp");
  AbstractCommandLineMenu menu({ alpha });

  FORBID_CALL(*alpha, printCommandList(ANY(std::set<std::string>)));
  menu.printCommandList();
  SUCCEED();
}

TEST_CASE("AbstractCommandLineMenu: printCommandList forwards the entries of the active component")
{
  auto alpha = acmComponent("Alpha", "Alp");
  AbstractCommandLineMenu menu({ alpha });

  REQUIRE(menu.onMenu("Alpha"));
  ALLOW_CALL(*alpha, menuEntriesOf()).LR_RETURN(std::set<std::string>{ "sub" });
  REQUIRE_CALL(*alpha, printCommandList(std::set<std::string>{ "sub" })).TIMES(1);

  menu.printCommandList();
}

}// namespace lintel
