#include <catch2/catch_all.hpp>

#include "lintel/features/cli/utils/CommandLineUtils.h"

namespace lintel {

TEST_CASE("CommandLineUtils: echo mode starts enabled")
{
  CommandLineUtils utils;
  REQUIRE(utils.isEchoMode());
}

TEST_CASE("CommandLineUtils: disableOfInputEcho turns echo off and is idempotent")
{
  CommandLineUtils utils;

  utils.disableOfInputEcho();
  REQUIRE_FALSE(utils.isEchoMode());

  utils.disableOfInputEcho();
  REQUIRE_FALSE(utils.isEchoMode());

  utils.enableOfInputEcho();
  REQUIRE(utils.isEchoMode());
}

TEST_CASE("CommandLineUtils: enableOfInputEcho can be called repeatedly and restores echo")
{
  CommandLineUtils utils;

  utils.enableOfInputEcho();
  utils.enableOfInputEcho();
  REQUIRE(utils.isEchoMode());

  utils.disableOfInputEcho();
  REQUIRE_FALSE(utils.isEchoMode());
}

TEST_CASE("CommandLineUtils: canonical mode starts disabled and toggles")
{
  CommandLineUtils utils;
  REQUIRE_FALSE(utils.isCanonicalMode());

  utils.disableOfCanonicalMode();
  REQUIRE_FALSE(utils.isCanonicalMode());

  utils.enableOfCanonicalMode();
  REQUIRE(utils.isCanonicalMode());
}

TEST_CASE("CommandLineUtils: clear and beep do not throw")
{
  REQUIRE_NOTHROW(CommandLineUtils::clear());
  REQUIRE_NOTHROW(CommandLineUtils::beep());
}

}// namespace lintel
