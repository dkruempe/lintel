#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include <memory>

#include "../mocks/MockMessageQueueRepository.h"
#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/models/ProcessName.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/configuration/MessageQueueComponent.h"
#include "lintel/features/base/configuration/MessageQueueEntry.h"
#include "lintel/features/base/services/MessageQueueService.h"

namespace lintel {
using namespace trompeloeil;

namespace {
  std::shared_ptr<Configuration> createTestConfiguration()
  {
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    envConfig->overrides(EnvironmentConfiguration::ConfigDirectory, "/nonexistent_config_dir");
    envConfig->overrides(EnvironmentConfiguration::BootstrapConfigName, "nonexistent");
    return std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, envConfig);
  }
}// namespace

TEST_CASE("MessageQueueService: can be constructed without db access")
{
  auto mockRepo = std::make_shared<MockMessageQueueRepository>();
  auto processName = std::make_shared<ProcessName>("testProcess");
  auto configuration = createTestConfiguration();

  ALLOW_CALL(*mockRepo, allOf(ANY(MessageQueueFilter))).LR_RETURN(std::vector<MessageQueueEntry>{});

  REQUIRE_NOTHROW(MessageQueueService(configuration, processName, mockRepo));
}

TEST_CASE("MessageQueueService: repository is queried on onInitialize")
{
  auto mockRepo = std::make_shared<MockMessageQueueRepository>();
  auto processName = std::make_shared<ProcessName>("testProcess");
  auto configuration = createTestConfiguration();

  REQUIRE_CALL(*mockRepo, allOf(ANY(MessageQueueFilter))).LR_RETURN(std::vector<MessageQueueEntry>{});

  MessageQueueService service(configuration, processName, mockRepo);
  service.onInitialize();
}

namespace {
  /** Configuration with a single queue "mq1" owned by testProcess. */
  std::shared_ptr<Configuration> createQueueConfiguration()
  {
    auto envConfig = std::make_shared<EnvironmentConfiguration>();
    envConfig->overrides(EnvironmentConfiguration::ConfigDirectory, "/nonexistent_config_dir");
    envConfig->overrides(EnvironmentConfiguration::BootstrapConfigName, "nonexistent");
    auto configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, envConfig);
    configuration->setEntries(
      { std::make_shared<MessageQueueEntry>(type_name<MessageQueueComponent>(), "testProcess", "mq1", 10) });
    return configuration;
  }
}// namespace

TEST_CASE("MessageQueueService: of() registers this process as the queue owner")
{
  auto mockRepo = std::make_shared<MockMessageQueueRepository>();
  auto processName = std::make_shared<ProcessName>("testProcess");
  auto configuration = createQueueConfiguration();

  ALLOW_CALL(*mockRepo, allOf(ANY(MessageQueueFilter))).LR_RETURN(std::vector<MessageQueueEntry>{});

  MessageQueueService service(configuration, processName, mockRepo);
  auto queue = service.of("mq1");

  REQUIRE(mockRepo->registeredOwners.size() == 1);
  REQUIRE(mockRepo->registeredOwners[0].starts_with("mq1/testProcess/"));
}

TEST_CASE("MessageQueueService: updateHeartbeat writes the heartbeat to the repository")
{
  auto mockRepo = std::make_shared<MockMessageQueueRepository>();
  auto processName = std::make_shared<ProcessName>("testProcess");
  auto configuration = createTestConfiguration();

  ALLOW_CALL(*mockRepo, allOf(ANY(MessageQueueFilter))).LR_RETURN(std::vector<MessageQueueEntry>{});

  MessageQueueService service(configuration, processName, mockRepo);
  service.updateHeartbeat("mq1");

  REQUIRE(mockRepo->heartbeats == std::vector<std::string>{ "mq1" });
}

TEST_CASE("MessageQueueService: recoverOrphans reclaims the entries the repository reports")
{
  auto mockRepo = std::make_shared<MockMessageQueueRepository>();
  auto processName = std::make_shared<ProcessName>("testProcess");
  auto configuration = createTestConfiguration();

  ALLOW_CALL(*mockRepo, allOf(ANY(MessageQueueFilter))).LR_RETURN(std::vector<MessageQueueEntry>{});
  // MessageQueueEntry is copyable but not assignable (const members), so no initializer-list assign
  mockRepo->orphansToFind.push_back(
    MessageQueueEntry(type_name<MessageQueueComponent>(), "otherProcess", "orphan1", 5));
  mockRepo->orphansToFind.push_back(
    MessageQueueEntry(type_name<MessageQueueComponent>(), "otherProcess", "orphan2", 5));

  MessageQueueService service(configuration, processName, mockRepo);

  REQUIRE(service.recoverOrphans(std::chrono::seconds(30)) == 2);
  REQUIRE(mockRepo->reclaimedQueues == std::vector<std::string>{ "orphan1", "orphan2" });
}

TEST_CASE("MessageQueueService: recoverOrphans without stale owners reclaims nothing")
{
  auto mockRepo = std::make_shared<MockMessageQueueRepository>();
  auto processName = std::make_shared<ProcessName>("testProcess");
  auto configuration = createTestConfiguration();

  ALLOW_CALL(*mockRepo, allOf(ANY(MessageQueueFilter))).LR_RETURN(std::vector<MessageQueueEntry>{});

  MessageQueueService service(configuration, processName, mockRepo);

  REQUIRE(service.recoverOrphans() == 0);
  REQUIRE(mockRepo->reclaimedQueues.empty());
}
}// namespace lintel
