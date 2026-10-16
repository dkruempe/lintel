#include <catch2/catch_all.hpp>

#include <cstdint>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "lintel/features/cli/models/Command.h"

namespace lintel {
namespace {

  std::string helpTextOf(const Command &command)
  {
    std::ostringstream stream;
    command.printHelp(stream);
    return stream.str();
  }

}// namespace

TEST_CASE("Command exposes its name")
{
  Command command("greet", "Greet somebody");
  REQUIRE(command.getCommand() == "greet");
}

TEST_CASE("Command::printHelp prints the description and arguments")
{
  bool verbose = false;
  std::string name;
  Command command("greet", "Greet somebody");
  command.addArgument({ "--verbose", "-v" }, &verbose, "Enable verbose output");
  command.addArgument({ "--name", "-n" }, &name, "Name to greet");

  const std::string help = helpTextOf(command);
  REQUIRE(help.find("greet:") != std::string::npos);
  REQUIRE(help.find("Greet somebody") != std::string::npos);
  REQUIRE(help.find("Arguments:") != std::string::npos);
  REQUIRE(help.find("--verbose") != std::string::npos);
  REQUIRE(help.find("Enable verbose output") != std::string::npos);
}

TEST_CASE("Command::printHelp wraps a long help text")
{
  bool flag = false;
  Command command("wrap", "Wrap long help");
  command.addArgument({ "--flag" },
    &flag,
    "this is a deliberately long help text that must exceed the sixty character wrap width so it wraps");
  const std::string help = helpTextOf(command);
  REQUIRE(help.find("--flag") != std::string::npos);
}

TEST_CASE("Command::printHelp without arguments omits the arguments section")
{
  Command command("plain", "No arguments here");
  const std::string help = helpTextOf(command);
  REQUIRE(help.find("Arguments:") == std::string::npos);
}

TEST_CASE("Command assigns both addArgument overloads")
{
  bool first = false;
  bool second = false;
  Command command("multi", "Overloads");
  command.addArgument({ "--first" }, &first, "first flag");
  command.addArgument(std::vector<std::string>{ "--second" }, &second, std::string("second flag"));

  command.parse("multi", { "--first", "--second" });
  REQUIRE(first);
  REQUIRE(second);
}

TEST_CASE("Command::parse handles space separated, equals separated and bool values")
{
  int32_t count = 0;
  std::string name;
  bool verbose = false;
  Command command("cmd", "parse");
  command.addArgument({ "--count", "-c" }, &count, "count");
  command.addArgument({ "--name" }, &name, "name");
  command.addArgument({ "--verbose" }, &verbose, "verbose");

  command.parse("cmd", { "--count=5", "--name", "alice", "--verbose" });
  REQUIRE(count == 5);
  REQUIRE(name == "alice");
  REQUIRE(verbose);

  // last one wins
  command.parse("cmd", { "--count", "9" });
  REQUIRE(count == 9);

  // explicit false
  command.parse("cmd", { "--verbose=false" });
  REQUIRE_FALSE(verbose);
}

TEST_CASE("Command::parse parses every supported value type")
{
  int32_t i32 = 0;
  uint32_t u32 = 0;
  double dbl = 0.0;
  float flt = 0.0F;
  std::string str;
  std::vector<std::string> list;
  Command command("types", "types");
  command.addArgument({ "--i32" }, &i32, "i32");
  command.addArgument({ "--u32" }, &u32, "u32");
  command.addArgument({ "--dbl" }, &dbl, "dbl");
  command.addArgument({ "--flt" }, &flt, "flt");
  command.addArgument({ "--str" }, &str, "str");
  command.addArgument({ "--list" }, &list, "list");

  command.parse(
    "types", { "--i32", "-42", "--u32", "7", "--dbl", "3.5", "--flt", "2.5", "--str=hello", "--list", "a,b,c" });
  REQUIRE(i32 == -42);
  REQUIRE(u32 == 7);
  REQUIRE(dbl == Catch::Approx(3.5));
  REQUIRE(flt == Catch::Approx(2.5F));
  REQUIRE(str == "hello");
  REQUIRE(list == std::vector<std::string>{ "a", "b", "c" });
}

TEST_CASE("Command::parse sets optional values and resetOptionals clears them")
{
  std::optional<int32_t> oi;
  std::optional<uint32_t> ou;
  std::optional<double> od;
  std::optional<float> of;
  std::optional<bool> ob;
  std::optional<std::string> os;
  Command command("opt", "opt");
  command.addArgument({ "--oi" }, &oi, "oi");
  command.addArgument({ "--ou" }, &ou, "ou");
  command.addArgument({ "--od" }, &od, "od");
  command.addArgument({ "--of" }, &of, "of");
  command.addArgument({ "--ob" }, &ob, "ob");
  command.addArgument({ "--os" }, &os, "os");

  command.parse("opt", { "--oi", "1", "--ou", "2", "--od", "3.5", "--of", "4.5", "--ob", "--os=text" });
  REQUIRE(oi.value() == 1);
  REQUIRE(ou.value() == 2);
  REQUIRE(od.value() == Catch::Approx(3.5));
  REQUIRE(of.value() == Catch::Approx(4.5F));
  REQUIRE(ob.value());
  REQUIRE(os.value() == "text");

  command.resetOptionals();
  REQUIRE_FALSE(oi.has_value());
  REQUIRE_FALSE(ou.has_value());
  REQUIRE_FALSE(od.has_value());
  REQUIRE_FALSE(of.has_value());
  REQUIRE_FALSE(ob.has_value());
  REQUIRE_FALSE(os.has_value());
}

TEST_CASE("Command::parse ignores unknown flags")
{
  bool known = false;
  Command command("cmd", "ignore");
  command.addArgument({ "--known" }, &known, "known");

  command.parse("cmd", { "--unknown", "--known" });
  REQUIRE(known);
}

TEST_CASE("Command::parse throws when a value is missing")
{
  int32_t count = 0;
  Command command("cmd", "missing");
  command.addArgument({ "--count" }, &count, "count");

  REQUIRE_THROWS_AS(command.parse("cmd", { "--count" }), std::runtime_error);
}

TEST_CASE("Command::parseValueFromStream parses a bare value into a target")
{
  int32_t value = 0;
  Command::parseValueFromStream("77", &value);
  REQUIRE(value == 77);
}

TEST_CASE("Command::parseValueFromStream parses optional string and bool targets")
{
  std::optional<std::string> optionalString;
  Command::parseValueFromStream("hello", &optionalString);
  REQUIRE(optionalString.value() == "hello");

  std::optional<bool> optionalBool;
  Command::parseValueFromStream("1", &optionalBool);
  REQUIRE(optionalBool.value());
}

TEST_CASE("Command::addArgument binds the lvalue flag list overload")
{
  bool third = false;
  // an lvalue vector selects the const-reference overload, a braced list selects the rvalue one
  std::vector<std::string> flags{ "--third", "-t" };
  Command command("lvalue", "lvalue");
  command.addArgument(flags, &third, "third flag");

  command.parse("lvalue", { "-t" });
  REQUIRE(third);
}
}// namespace lintel
