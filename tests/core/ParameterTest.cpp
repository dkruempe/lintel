#include <catch2/catch_all.hpp>

#include "lintel/core/persistence/Parameter.h"

namespace lintel {
namespace db {

TEST_CASE("Parameter: exposes value and length")
{
  const Parameter parameter = Parameter::with("hello");
  REQUIRE(parameter.getValue() == "hello");
  REQUIRE(parameter.getLength() == 5U);
  REQUIRE(Parameter::getFormatId() == 1);
}

TEST_CASE("Parameters: constructs from string vector and provides accessors")
{
  const std::vector<std::string> values{ "a", "bb" };
  const Parameters params(values);

  const auto cstrings = params.getParameters();
  REQUIRE(cstrings.size() == 2U);
  REQUIRE(cstrings[0] == std::string("a"));
  REQUIRE(cstrings[1] == std::string("bb"));

  const auto lengths = params.getParametersLengths();
  REQUIRE(lengths.size() == 2U);
  REQUIRE(lengths[0] == 1);
  REQUIRE(lengths[1] == 2);

  const auto types = params.getParametersTypes();
  REQUIRE(types.size() == 2U);
  REQUIRE(types[0] == 1);
  REQUIRE(types[1] == 1);
}

TEST_CASE("Parameters: constructs from parameter vector")
{
  std::vector<Parameter> parameters;
  parameters.push_back(Parameter::with("test"));
  const Parameters params(std::move(parameters));

  const auto cstrings = params.getParameters();
  REQUIRE(cstrings.size() == 1U);
  REQUIRE(std::string(cstrings[0]) == "test");
  REQUIRE(params.getParametersLengths()[0] == 4);
  REQUIRE(params.getParametersTypes()[0] == 1);
}

}// namespace db
}// namespace lintel
