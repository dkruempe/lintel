#include <lintel/features/base/BaseFeature.h>
#include <lintel/features/base/configuration/Configuration.h>
#include <lintel/features/base/configuration/EnvironmentConfiguration.h>
#include <lintel/features/base/configuration/HistoryComponent.h>
#include <lintel/features/base/configuration/HistoryServiceEntry.h>
#include <lintel/features/base/configuration/MessageQueueComponent.h>
#include <lintel/features/base/configuration/MessageQueueEntry.h>
#include <lintel/features/base/models/ProcessName.h>
#include <lintel/features/base/services/HistoryService.h>
#include <lintel/features/base/services/IHistoryService.h>
#include <lintel/features/base/services/NoopHistoryService.h>
#include <lintel/features/Features.h>
#include <lintel/core/utils/TypeName.h>

#include "Hypodermic/ContainerBuilder.h"
#include "Hypodermic/Container.h"

#include <catch2/catch_all.hpp>

#include <cstdlib>
#include <memory>
#include <vector>

#include "../helpers/ScopedEnvironmentVariable.h"

namespace {

std::shared_ptr<Configuration> configurationWith(
        const std::vector<std::shared_ptr<Entry>> &entries) {
  // Local guard: it only has to outlive the EnvironmentConfiguration built below, not the whole
  // test case.
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test" };
    auto config = std::make_shared<Configuration>(
            std::vector<std::shared_ptr<Component>>{},
            std::make_shared<EnvironmentConfiguration>());
    config->setEntries(entries);
    return config;
}

std::shared_ptr<HistoryServiceEntry> historyEntry(const std::string &processName) {
    return std::make_shared<HistoryServiceEntry>(
            type_name<HistoryComponent>(), processName, "history", 1000);
}

}  // namespace

TEST_CASE("HistoryServiceGating: non-owner with history config gets a HistoryService (producer only)") {
    auto config = configurationWith({
            historyEntry("main"),
            std::make_shared<MessageQueueEntry>(
                    type_name<MessageQueueComponent>(), "main", "history", 1000)});
    auto processName = std::make_shared<ProcessName>("worker");

    Hypodermic::ContainerBuilder builder;
    auto feature = std::make_shared<BaseFeature>(std::make_shared<Features>());
    builder.registerInstance(processName);
    builder.registerInstance(config);
    feature->registerTypes(builder, config, processName);
    auto container = builder.build();

    auto historyService = container->resolve<IHistoryService>();
    REQUIRE(historyService != nullptr);
    REQUIRE(std::dynamic_pointer_cast<HistoryService>(historyService) != nullptr);
    REQUIRE(std::dynamic_pointer_cast<NoopHistoryService>(historyService) == nullptr);
}

TEST_CASE("HistoryServiceGating: process without any history configuration gets a NoopHistoryService") {
    auto config = configurationWith({});
    auto processName = std::make_shared<ProcessName>("main");

    Hypodermic::ContainerBuilder builder;
    auto feature = std::make_shared<BaseFeature>(std::make_shared<Features>());
    builder.registerInstance(processName);
    builder.registerInstance(config);
    feature->registerTypes(builder, config, processName);
    auto container = builder.build();

    auto historyService = container->resolve<IHistoryService>();
    REQUIRE(historyService != nullptr);
    REQUIRE(std::dynamic_pointer_cast<NoopHistoryService>(historyService) != nullptr);
}

TEST_CASE("HistoryServiceGating: configured owner process gets the real HistoryService") {
    auto config = configurationWith({
            historyEntry("main"),
            std::make_shared<MessageQueueEntry>(
                    type_name<MessageQueueComponent>(), "main", "history", 1000)});
    auto processName = std::make_shared<ProcessName>("main");

    Hypodermic::ContainerBuilder builder;
    auto feature = std::make_shared<BaseFeature>(std::make_shared<Features>());
    builder.registerInstance(processName);
    builder.registerInstance(config);
    feature->registerTypes(builder, config, processName);
    auto container = builder.build();

    auto historyService = container->resolve<IHistoryService>();
    REQUIRE(historyService != nullptr);
    REQUIRE(std::dynamic_pointer_cast<HistoryService>(historyService) != nullptr);
    REQUIRE(std::dynamic_pointer_cast<NoopHistoryService>(historyService) == nullptr);
}
