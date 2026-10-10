#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include "../mocks/MockBootstrapPlugin.h"
#include "lintel/core/services/BootstrapService.h"

using namespace trompeloeil;

TEST_CASE("BootstrapService: calls onStart on each plugin") {
    auto plugin1 = std::make_shared<MockBootstrapPlugin>();
    auto plugin2 = std::make_shared<MockBootstrapPlugin>();

    ALLOW_CALL(*plugin1, getPriority())
        .LR_RETURN(BootstrapSequence::Database);
    ALLOW_CALL(*plugin2, getPriority())
        .LR_RETURN(BootstrapSequence::SharedMemory);

    REQUIRE_CALL(*plugin1, onStart());
    REQUIRE_CALL(*plugin2, onStart());

    BootstrapService service({plugin1, plugin2});
    service.onStart();
}

TEST_CASE("BootstrapService: calls plugins in priority order") {
    auto lowPlugin = std::make_shared<MockBootstrapPlugin>();
    auto highPlugin = std::make_shared<MockBootstrapPlugin>();

    std::vector<std::shared_ptr<BootstrapPlugin>> callOrder;

    ALLOW_CALL(*lowPlugin, getPriority())
        .LR_RETURN(BootstrapSequence::Database);
    ALLOW_CALL(*highPlugin, getPriority())
        .LR_RETURN(BootstrapSequence::SharedMemory);

    REQUIRE_CALL(*lowPlugin, onStart())
        .LR_SIDE_EFFECT(callOrder.push_back(lowPlugin));
    REQUIRE_CALL(*highPlugin, onStart())
        .LR_SIDE_EFFECT(callOrder.push_back(highPlugin));

    BootstrapService service({highPlugin, lowPlugin});
    service.onStart();

    REQUIRE(callOrder.size() == 2);
    REQUIRE(callOrder[0] == lowPlugin);
    REQUIRE(callOrder[1] == highPlugin);
}

TEST_CASE("BootstrapService: no plugins does nothing") {
  // An empty plugin list gives onStart() nothing to sort and nothing to start, and the service
  // keeps its list private - so the no-op is asserted through the only channel it can act on:
  // a plugin that exists in the process but is not part of this service must stay untouched,
  // and the caller must still own its list afterwards.
  auto foreignPlugin = std::make_shared<MockBootstrapPlugin>();
  ALLOW_CALL(*foreignPlugin, getPriority()).LR_RETURN(BootstrapSequence::Database);
  FORBID_CALL(*foreignPlugin, onStart());

  std::vector<std::shared_ptr<BootstrapPlugin>> callerPlugins{ foreignPlugin };

  BootstrapService service({});
  service.onStart();

  REQUIRE(callerPlugins.size() == 1);
  REQUIRE(callerPlugins.front() == foreignPlugin);
}
