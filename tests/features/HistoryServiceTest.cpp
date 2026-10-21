#include <catch2/catch_all.hpp>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "../helpers/PlatformCompat.h"

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/DatabaseConnectionComponent.h"
#include "lintel/core/configuration/DatabaseConnectionEntry.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/models/ProcessName.h"
#include "lintel/core/persistence/Connection.h"
#include "lintel/core/persistence/ConnectionType.h"
#include "lintel/core/persistence/DatabaseConnectionConfigurations.h"
#include "lintel/core/persistence/Statement.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/configuration/MessageQueueComponent.h"
#include "lintel/features/base/configuration/MessageQueueEntry.h"
#include "lintel/features/base/models/HistoryEntry.h"
#include "lintel/features/base/msg/Message.h"
#include "lintel/features/base/msg/MessageQueue.h"
#include "lintel/features/base/repositories/HistoryRepository.h"
#include "lintel/features/base/services/HistoryService.h"
#include "lintel/features/base/services/MessageQueueService.h"
#include "lintel/features/base/services/SchedulerService.h"

#include "../helpers/ScopedEnvironmentVariable.h"
#include "../mocks/MockMessageQueueRepository.h"

namespace lintel {
namespace {

  // `hs` prefix: unity builds merge the test files into one anonymous namespace.
  constexpr char hsQueueName[] = "history";

  std::chrono::sys_time<std::chrono::microseconds> hsTimestampOf(int seconds)
  {
    return std::chrono::sys_time<std::chrono::microseconds>{ std::chrono::microseconds{
      std::chrono::seconds{ seconds } } };
  }

  struct HistoryServiceFixture
  {
    // must stay first: set before the EnvironmentConfiguration below reads it
    ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_history_service_test_cfg" };
    std::string m_dbPath;
    std::shared_ptr<EnvironmentConfiguration> m_envConfig;
    std::shared_ptr<Configuration> m_configuration;
    std::shared_ptr<DatabaseConnectionConfigurations> m_connectionConfigurations;
    std::shared_ptr<HistoryRepository> m_historyRepository;
    std::shared_ptr<SchedulerService> m_scheduler;

    explicit HistoryServiceFixture(const std::string &queueOwnerProcess = "histowner")
    {
      static int counter = 0;
      m_dbPath = (std::filesystem::temp_directory_path()
                  / ("history_service_test_" + std::to_string(::getpid()) + "_" + std::to_string(++counter) + ".db"))
                   .string();
      std::remove(m_dbPath.c_str());

      db::Connection connection(db::ConnectionType::SQLite, m_dbPath);
      db::Statement statement(connection);
      statement.execute(R"(
            CREATE TABLE history (
                process_name       TEXT NOT NULL,
                service_name       TEXT NOT NULL,
                label              TEXT NOT NULL,
                text               TEXT NOT NULL,
                uuid               TEXT NOT NULL,
                created_timestamp  TEXT NOT NULL,
                inserted_timestamp TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                CONSTRAINT history_PK PRIMARY KEY (process_name, service_name, label, uuid)
            );
        )");

      m_envConfig = std::make_shared<EnvironmentConfiguration>();
      m_configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, m_envConfig);
      m_configuration->setEntries(std::vector<std::shared_ptr<Entry>>{
        std::make_shared<DatabaseConnectionEntry>(type_name<DatabaseConnectionComponent>(),
          m_dbPath,
          "",
          "",
          db::ConnectionType::SQLite,
          "default",
          0,
          "",
          true),
        std::make_shared<MessageQueueEntry>(
          type_name<MessageQueueComponent>(), queueOwnerProcess, hsQueueName, 1024) });
      m_connectionConfigurations = std::make_shared<DatabaseConnectionConfigurations>(m_configuration);
      m_historyRepository = std::make_shared<HistoryRepository>(m_connectionConfigurations);
      m_scheduler = std::make_shared<SchedulerService>(std::make_shared<ProcessName>("history-service-test"));

      // start from a clean, unowned queue
      MessageQueue<Message>::removeOf(hsQueueName);
    }

    ~HistoryServiceFixture()
    {
      m_historyRepository.reset();
      m_connectionConfigurations.reset();
      m_configuration.reset();
      m_scheduler.reset();
      std::remove(m_dbPath.c_str());
      MessageQueue<Message>::removeOf(hsQueueName);
    }

    HistoryServiceFixture(const HistoryServiceFixture &) = delete;
    HistoryServiceFixture &operator=(const HistoryServiceFixture &) = delete;

    std::shared_ptr<MessageQueueService> messageQueueService(const std::string &queueProcess) const
    {
      auto repository = std::make_shared<MockMessageQueueRepository>();
      ALLOW_CALL(*repository, allOf(ANY(MessageQueueFilter))).LR_RETURN(std::vector<MessageQueueEntry>{});
      return std::make_shared<MessageQueueService>(
        m_configuration, std::make_shared<ProcessName>(queueProcess), repository);
    }
  };

  HistoryEntry
    hsEntry(const std::string &process, const std::string &service, const std::string &label, const std::string &text)
  {
    return HistoryEntry(process, service, label, text, hsTimestampOf(100));
  }

}// namespace

TEST_CASE("HistoryService: getProcessName returns the owning process")
{
  HistoryServiceFixture fixture;
  auto service = std::make_shared<HistoryService>(std::make_shared<ProcessName>("histproc"),
    fixture.m_scheduler,
    fixture.m_historyRepository,
    fixture.messageQueueService("histproc"));

  REQUIRE(service->getProcessName() == "histproc");
}

TEST_CASE("HistoryService: allOf query overloads delegate to the repository")
{
  HistoryServiceFixture fixture;
  fixture.m_historyRepository->insertOf(
    { hsEntry("p1", "s1", "labelA", "text-1"), hsEntry("p1", "s2", "labelB", "text-2") });

  auto service = std::make_shared<HistoryService>(std::make_shared<ProcessName>("histproc"),
    fixture.m_scheduler,
    fixture.m_historyRepository,
    fixture.messageQueueService("histproc"));

  REQUIRE(service->allOf().size() == 2);
  REQUIRE(service->allOf("labelA").size() == 1);
  REQUIRE(service->allOf("p1", "s1").size() == 1);
  REQUIRE(service->allOfProcess("p1").size() == 2);
  REQUIRE(service->allOfService("s2").size() == 1);
}

TEST_CASE("HistoryService: historizeOf with no entries is a no-op")
{
  HistoryServiceFixture fixture;
  auto service = std::make_shared<HistoryService>(std::make_shared<ProcessName>("histproc"),
    fixture.m_scheduler,
    fixture.m_historyRepository,
    fixture.messageQueueService("histproc"));

  REQUIRE_NOTHROW(service->historizeOf({}));
  REQUIRE(fixture.m_historyRepository->allOf().empty());
}

TEST_CASE("HistoryService: owner buffers entries and flushes them on shutdown")
{
  HistoryServiceFixture fixture;
  auto service = std::make_shared<HistoryService>(std::make_shared<ProcessName>("histowner"),
    fixture.m_scheduler,
    fixture.m_historyRepository,
    fixture.messageQueueService("histowner"));

  REQUIRE(fixture.m_historyRepository->allOf().empty());
  service->historizeOf({ hsEntry("histowner", "svc", "buffered", "buffered-text") });
  // not committed yet: the owner only buffers
  REQUIRE(fixture.m_historyRepository->allOf().empty());

  service->onShutdown();
  service.reset();

  REQUIRE(fixture.m_historyRepository->allOf().size() == 1);
}

TEST_CASE("HistoryService: non-owner sends entries through the queue and drains them on shutdown")
{
  HistoryServiceFixture fixture("otherproc");
  auto service = std::make_shared<HistoryService>(std::make_shared<ProcessName>("histworker"),
    fixture.m_scheduler,
    fixture.m_historyRepository,
    fixture.messageQueueService("histworker"));

  service->historizeOf(
    { hsEntry("histworker", "svcA", "queued-a", "text-a"), hsEntry("histworker", "svcB", "queued-b", "text-b") });
  REQUIRE(fixture.m_historyRepository->allOf().empty());

  service.reset();

  REQUIRE(fixture.m_historyRepository->allOf().size() == 2);
}

TEST_CASE("HistoryService: onInitialize schedules the owner and returns for a non-owner")
{
  HistoryServiceFixture ownerFixture;
  auto ownerService = std::make_shared<HistoryService>(std::make_shared<ProcessName>("histowner"),
    ownerFixture.m_scheduler,
    ownerFixture.m_historyRepository,
    ownerFixture.messageQueueService("histowner"));
  REQUIRE_NOTHROW(ownerService->onInitialize());
  ownerService->onShutdown();
  ownerService.reset();

  HistoryServiceFixture workerFixture("otherproc");
  auto workerService = std::make_shared<HistoryService>(std::make_shared<ProcessName>("histworker"),
    workerFixture.m_scheduler,
    workerFixture.m_historyRepository,
    workerFixture.messageQueueService("histworker"));
  REQUIRE_NOTHROW(workerService->onInitialize());
  workerService->onShutdown();
  workerService.reset();

  SUCCEED();
}

}// namespace lintel
