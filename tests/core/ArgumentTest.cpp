#include <catch2/catch_all.hpp>

#include <cstdint>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "lintel/core/utils/Argument.h"

namespace lintel {

TEST_CASE("Argument: printHelp writes flag, short flag and description")
{
  int32_t value = 0;
  Argument argument("--count", "-c", "a counter", &value);

  std::ostringstream stream;
  argument.printHelp(stream);

  REQUIRE(stream.str() == "\t--count, -c a counter\n");
  REQUIRE(argument.getFlag() == "--count");
  REQUIRE(argument.getShortFlag() == "-c");
}

TEST_CASE("Argument: parse into a plain int32_t")
{
  int32_t value = 0;
  Argument argument("--count", "-c", "", &value);
  argument.parse("-42");
  REQUIRE(value == -42);
}

TEST_CASE("Argument: parse into a plain uint32_t")
{
  uint32_t value = 0;
  Argument argument("--count", "-c", "", &value);
  argument.parse("7");
  REQUIRE(value == 7U);
}

TEST_CASE("Argument: parse into a plain double")
{
  double value = 0.0;
  Argument argument("--ratio", "-r", "", &value);
  argument.parse("1.5");
  REQUIRE(value == 1.5);
}

TEST_CASE("Argument: parse into a plain float")
{
  float value = 0.0F;
  Argument argument("--ratio", "-r", "", &value);
  argument.parse("2.5");
  REQUIRE(value == 2.5F);
}

TEST_CASE("Argument: parse into a string")
{
  std::string value;
  Argument argument("--name", "-n", "", &value);
  argument.parse("hello");
  REQUIRE(value == "hello");
}

TEST_CASE("Argument: bool accepts true/false/empty and ignores other values")
{
  bool value = false;
  Argument argument("--verbose", "-v", "", &value);

  argument.parse("true");
  REQUIRE(value);

  argument.parse("false");
  REQUIRE_FALSE(value);

  // an empty argument means "the flag is present"
  argument.parse("");
  REQUIRE(value);

  // anything else leaves the value untouched
  value = false;
  argument.parse("maybe");
  REQUIRE_FALSE(value);
}

TEST_CASE("Argument: empty value for a non-bool argument throws")
{
  std::string value;
  Argument argument("--name", "-n", "", &value);
  REQUIRE_THROWS_AS(argument.parse(""), std::runtime_error);
}

TEST_CASE("Argument: optional string is reset and set on each parse")
{
  std::optional<std::string> value;
  Argument argument("--name", "-n", "", &value);

  argument.parse("first");
  REQUIRE(value.has_value());
  REQUIRE(*value == "first");

  argument.parse("second");
  REQUIRE(value.has_value());
  REQUIRE(*value == "second");
}

TEST_CASE("Argument: optional bool mirrors the bool rules")
{
  std::optional<bool> value;
  Argument argument("--flag", "-f", "", &value);

  argument.parse("true");
  REQUIRE(value.has_value());
  REQUIRE(*value);

  argument.parse("false");
  REQUIRE(value.has_value());
  REQUIRE_FALSE(*value);

  argument.parse("");
  REQUIRE(value.has_value());
  REQUIRE(*value);
}

TEST_CASE("Argument: optional numerics are parsed from the stream")
{
  std::optional<int32_t> intValue;
  std::optional<uint32_t> uintValue;
  std::optional<double> doubleValue;
  std::optional<float> floatValue;

  Argument intArgument("--int", "-i", "", &intValue);
  Argument uintArgument("--uint", "-u", "", &uintValue);
  Argument doubleArgument("--double", "-d", "", &doubleValue);
  Argument floatArgument("--float", "-f", "", &floatValue);

  intArgument.parse("-7");
  uintArgument.parse("7");
  doubleArgument.parse("3.25");
  floatArgument.parse("1.25");

  REQUIRE(intValue.has_value());
  REQUIRE(*intValue == -7);
  REQUIRE(uintValue.has_value());
  REQUIRE(*uintValue == 7U);
  REQUIRE(doubleValue.has_value());
  REQUIRE(*doubleValue == 3.25);
  REQUIRE(floatValue.has_value());
  REQUIRE(*floatValue == 1.25F);
}

TEST_CASE("ArgumentProvider: provide returns the supplied arguments")
{
  int32_t value = 0;
  std::vector<Argument> arguments{ Argument("--count", "-c", "counter", &value) };
  ArgumentProvider provider(arguments);

  REQUIRE(provider.provide().size() == 1);
  REQUIRE(provider.provide()[0].getFlag() == "--count");
}

}// namespace lintel
