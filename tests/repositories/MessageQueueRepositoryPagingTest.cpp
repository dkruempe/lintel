#include <lintel/core/configuration/Configuration.h>
#include <lintel/core/configuration/DatabaseConnectionComponent.h>
#include <lintel/core/configuration/DatabaseConnectionEntry.h>
#include <lintel/core/configuration/EnvironmentConfiguration.h>
#include <lintel/core/exceptions/SQLException.h>
#include <lintel/core/persistence/Connection.h>
#include <lintel/core/persistence/DatabaseConnectionConfigurations.h>
#include <lintel/core/persistence/Statement.h>
#include <lintel/features/base/configuration/MessageQueueEntry.h>
#include <lintel/features/base/repositories/MessageQueueRepository.h>

#include <catch2/catch_all.hpp>

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "../helpers/PlatformCompat.h"

#include "../helpers/ScopedEnvironmentVariable.h"

namespace lintel {
namespace {

  std::string kMqPagingDb()
  {
    static int counter = 0;
    return (std::filesystem::temp_directory_path()
            / ("mq_paging_test_" + std::to_string(::getpid()) + "_" + std::to_string(counter++) + ".db"))
      .string();
  }

  // m_configDirectory must be the first member: members initialize in declaration order, so the guard
  // sets CONFIG_DIRECTORY before the constructor body builds the EnvironmentConfiguration, and it is
  // destroyed last - after the test case is done with it.
  struct MessageQueuePagingFixture
  {
    ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_mq_paging_cfg" };
    std::string m_dbPath;
    std::shared_ptr<EnvironmentConfiguration> envConfig;
    std::shared_ptr<Configuration> configuration;
    std::shared_ptr<DatabaseConnectionConfigurations> connectionConfigurations;
    std::shared_ptr<MessageQueueRepository> repository;

    MessageQueuePagingFixture() : m_dbPath(kMqPagingDb())
    {
      std::remove(m_dbPath.c_str());

      db::Connection connection(db::ConnectionType::SQLite, m_dbPath);
      db::Statement statement(connection);
      statement.execute(R"(
            CREATE TABLE message_queues (
                name text NOT NULL,
                process_name text NOT NULL,
                max_messages integer NOT NULL,
                owner_pid INTEGER,
                owner_host text,
                started_at timestamp,
                last_heartbeat_at timestamp,
                owner_state text,
                CONSTRAINT message_queues_pk PRIMARY KEY (name)
            );
        )");

      envConfig = std::make_shared<EnvironmentConfiguration>();
      configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, envConfig);
      auto entry = std::make_shared<DatabaseConnectionEntry>(
        type_name<DatabaseConnectionComponent>(), m_dbPath, "", "", db::ConnectionType::SQLite, "default", 0, "", true);
      configuration->setEntries(std::vector<std::shared_ptr<Entry>>{ entry });
      connectionConfigurations = std::make_shared<DatabaseConnectionConfigurations>(configuration);
      repository = std::make_shared<MessageQueueRepository>(connectionConfigurations);
    }

    ~MessageQueuePagingFixture() { std::remove(m_dbPath.c_str()); }

    void insertQueue(const std::string &process, const std::string &queue, int32_t maxMessages = 10)
    {
      repository->insertOf({ MessageQueueEntry("test", process, queue, maxMessages) });
    }
  };

  /** Builds a repository from an arbitrary set of connection entries. */
  struct MessageQueueConfigFixture
  {
    ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_mq_unsupported_cfg" };
    std::shared_ptr<EnvironmentConfiguration> envConfig;
    std::shared_ptr<Configuration> configuration;
    std::shared_ptr<MessageQueueRepository> repository;

    void withEntries(std::vector<std::shared_ptr<Entry>> entries)
    {
      envConfig = std::make_shared<EnvironmentConfiguration>();
      configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, envConfig);
      configuration->setEntries(std::move(entries));
      repository =
        std::make_shared<MessageQueueRepository>(std::make_shared<DatabaseConnectionConfigurations>(configuration));
    }
  };

}// namespace

TEST_CASE("MessageQueueRepository::pageOf walks through all queues", "[message_queue_paging]")
{
  MessageQueuePagingFixture fixture;
  for (const char *queue : { "queue_a", "queue_b", "queue_c", "queue_d", "queue_e" }) {
    fixture.insertQueue("proc1", queue);
  }

  const Page<MessageQueueEntry> page1 = fixture.repository->pageOf(MessageQueueFilter{ ".*", ".*" }, std::nullopt, 2);
  REQUIRE(page1.getItems().size() == 2);
  REQUIRE(page1.getItems()[0].get_message_queue_name() == "queue_a");
  REQUIRE(page1.getItems()[1].get_message_queue_name() == "queue_b");
  REQUIRE(page1.hasMore());
  REQUIRE(page1.getNextAfter().has_value());
  REQUIRE(page1.getNextAfter().value() == "queue_b");

  const Page<MessageQueueEntry> page2 =
    fixture.repository->pageOf(MessageQueueFilter{ ".*", ".*" }, page1.getNextAfter(), 2);
  REQUIRE(page2.getItems().size() == 2);
  REQUIRE(page2.getItems()[0].get_message_queue_name() == "queue_c");
  REQUIRE(page2.getItems()[1].get_message_queue_name() == "queue_d");
  REQUIRE(page2.hasMore());

  const Page<MessageQueueEntry> page3 =
    fixture.repository->pageOf(MessageQueueFilter{ ".*", ".*" }, page2.getNextAfter(), 2);
  REQUIRE(page3.getItems().size() == 1);
  REQUIRE(page3.getItems()[0].get_message_queue_name() == "queue_e");
  REQUIRE_FALSE(page3.hasMore());
  REQUIRE_FALSE(page3.getNextAfter().has_value());
}

TEST_CASE("MessageQueueRepository::pageOf respects process filter", "[message_queue_paging]")
{
  MessageQueuePagingFixture fixture;
  fixture.insertQueue("proc1", "queue_a");
  fixture.insertQueue("proc2", "queue_b");
  fixture.insertQueue("proc1", "queue_c");

  const Page<MessageQueueEntry> page =
    fixture.repository->pageOf(MessageQueueFilter{ "proc1", ".*" }, std::nullopt, 10);
  REQUIRE(page.getItems().size() == 2);
  REQUIRE_FALSE(page.hasMore());
}

TEST_CASE("MessageQueueRepository::pageOf respects queue name filter", "[message_queue_paging]")
{
  MessageQueuePagingFixture fixture;
  fixture.insertQueue("proc1", "alpha_1");
  fixture.insertQueue("proc1", "beta_1");
  fixture.insertQueue("proc1", "beta_2");

  const Page<MessageQueueEntry> page =
    fixture.repository->pageOf(MessageQueueFilter{ ".*", "beta_.*" }, std::nullopt, 10);
  REQUIRE(page.getItems().size() == 2);
  REQUIRE(page.getItems()[0].get_message_queue_name() == "beta_1");
  REQUIRE(page.getItems()[1].get_message_queue_name() == "beta_2");
}

TEST_CASE("MessageQueueRepository::pageOf returns empty page without matches", "[message_queue_paging]")
{
  MessageQueuePagingFixture fixture;
  fixture.insertQueue("proc1", "queue_a");

  const Page<MessageQueueEntry> page =
    fixture.repository->pageOf(MessageQueueFilter{ "unknown_proc", ".*" }, std::nullopt, 5);
  REQUIRE(page.getItems().empty());
  REQUIRE_FALSE(page.hasMore());
  REQUIRE_FALSE(page.getNextAfter().has_value());
}

TEST_CASE("MessageQueueRepository::deleteOf removes the given queues", "[message_queue_paging]")
{
  MessageQueuePagingFixture fixture;
  fixture.insertQueue("proc1", "queue_a");
  fixture.insertQueue("proc1", "queue_b");
  fixture.insertQueue("proc2", "queue_c");

  fixture.repository->deleteOf(
    { MessageQueueEntry("test", "proc1", "queue_a", 10), MessageQueueEntry("test", "proc1", "queue_b", 10) });

  const auto remaining = fixture.repository->allOf(MessageQueueFilter{ ".*", ".*" });
  REQUIRE(remaining.size() == 1);
  REQUIRE(remaining[0].get_message_queue_name() == "queue_c");
}

TEST_CASE("MessageQueueRepository: queries without a default connection return empty", "[message_queue_paging]")
{
  MessageQueueConfigFixture fixture;
  fixture.withEntries({});

  REQUIRE(fixture.repository->allOf(MessageQueueFilter{ ".*", ".*" }).empty());

  const Page<MessageQueueEntry> page = fixture.repository->pageOf(MessageQueueFilter{ ".*", ".*" }, std::nullopt, 10);
  REQUIRE(page.getItems().empty());
  REQUIRE_FALSE(page.hasMore());

  // a limit of zero short-circuits before the connection is even consulted
  const Page<MessageQueueEntry> zero = fixture.repository->pageOf(MessageQueueFilter{ ".*", ".*" }, std::nullopt, 0);
  REQUIRE(zero.getItems().empty());
  REQUIRE_FALSE(zero.hasMore());
}

TEST_CASE("MessageQueueRepository: an unsupported connection type throws", "[message_queue_paging]")
{
  MessageQueueConfigFixture fixture;
  auto entry = std::make_shared<DatabaseConnectionEntry>(type_name<DatabaseConnectionComponent>(),
    "/tmp/mq_unsupported.db",
    "",
    "",
    db::ConnectionType(db::ConnectionType::UNDEFINED),
    "default",
    0,
    "",
    true);
  fixture.withEntries({ entry });

  REQUIRE_THROWS_AS(fixture.repository->allOf(MessageQueueFilter{ ".*", ".*" }), db::SQLException);
  REQUIRE_THROWS_AS(fixture.repository->pageOf(MessageQueueFilter{ ".*", ".*" }, std::nullopt, 5), db::SQLException);
  REQUIRE_THROWS_AS(fixture.repository->allMessageQueueNameOf("queue_a"), db::SQLException);
}
}// namespace lintel
