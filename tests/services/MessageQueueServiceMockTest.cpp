#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include <memory>

#include "../mocks/MockMessageQueueRepository.h"
#include "base_library/core/models/ProcessName.h"
#include "base_library/core/configuration/Configuration.h"
#include "base_library/core/configuration/EnvironmentConfiguration.h"
#include "base_library/core/configuration/MessageQueueComponent.h"
#include "base_library/core/configuration/MessageQueueEntry.h"
#include "base_library/features/base/services/MessageQueueService.h"

using namespace trompeloeil;

namespace {
    std::shared_ptr<Configuration> createTestConfiguration() {
        auto envConfig = std::make_shared<EnvironmentConfiguration>();
        envConfig->overrides(EnvironmentConfiguration::ConfigDirectory, "/nonexistent_config_dir");
        envConfig->overrides(EnvironmentConfiguration::BootstrapConfigName, "nonexistent");
        return std::make_shared<Configuration>(
            std::vector<std::shared_ptr<Component>>{}, envConfig);
    }
}

TEST_CASE("MessageQueueService: can be constructed without db access") {
    auto mockRepo = std::make_shared<MockMessageQueueRepository>();
    auto processName = std::make_shared<ProcessName>("testProcess");
    auto configuration = createTestConfiguration();

    ALLOW_CALL(*mockRepo, allOf(".*", ".*"))
        .LR_RETURN(std::vector<MessageQueueEntry>{});

    REQUIRE_NOTHROW(MessageQueueService(configuration, processName, mockRepo));
}

TEST_CASE("MessageQueueService: repository is queried on onInitialize") {
    auto mockRepo = std::make_shared<MockMessageQueueRepository>();
    auto processName = std::make_shared<ProcessName>("testProcess");
    auto configuration = createTestConfiguration();

    REQUIRE_CALL(*mockRepo, allOf(".*", ".*"))
        .LR_RETURN(std::vector<MessageQueueEntry>{});

    MessageQueueService service(configuration, processName, mockRepo);
    service.onInitialize();
}
