#include <chrono>
#include <sstream>
#include <string>

#include <catch2/catch_all.hpp>

#include "lintel/core/services/StopWatchService.h"

namespace lintel {

TEST_CASE("StopWatchService: a running stop watch measures elapsed time")
{
  StopWatchService service;
  const auto elapsed = service.elapsed();
  REQUIRE(elapsed.count() >= 0);

  service.reset();
  REQUIRE(service.elapsed().count() >= 0);

  service.stop();
  const auto stopped = service.elapsed();
  REQUIRE(stopped.count() >= 0);

  // stopping twice keeps the first measurement
  service.stop();
  REQUIRE(service.elapsed() == stopped);
}

TEST_CASE("StopWatchService: a stop watch constructed without running stays at zero")
{
  StopWatchService service(false);
  REQUIRE(service.elapsed().count() == 0);
}

TEST_CASE("StopWatchService: stream insertion prints milliseconds")
{
  StopWatchService service(false);
  std::ostringstream stream;
  stream << service;
  REQUIRE(stream.str().find(" ms") != std::string::npos);
}

}// namespace lintel
