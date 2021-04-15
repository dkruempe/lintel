#define CATCH_CONFIG_MAIN
#include <base_library/core/services/ProcessService.h>

#include <catch2/catch.hpp>

TEST_CASE("ProcessService Start Process") {
  Process process(0, "ls", "", {"-la", "/"}, true, false);
  ProcessService processService({process}, std::chrono::seconds(1),
                                std::chrono::milliseconds(100));
  REQUIRE(!processService.allProcesses().empty());
}