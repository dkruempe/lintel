#include <lintel/core/exceptions/SQLException.h>
#include <lintel/core/persistence/Connection.h>
#include <lintel/core/persistence/DatabaseConnectionConfigurations.h>
#include <lintel/core/persistence/Statement.h>
#include <lintel/features/base/configuration/Configuration.h>
#include <lintel/features/base/configuration/DatabaseConnectionComponent.h>
#include <lintel/features/base/configuration/DatabaseConnectionEntry.h>
#include <lintel/features/base/configuration/EnvironmentConfiguration.h>
#include <lintel/features/base/configuration/MessageQueueEntry.h>
#include <lintel/features/base/repositories/MessageQueueRepository.h>

#include <catch2/catch_all.hpp>

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "../helpers/ScopedEnvironmentVariable.h"

namespace {

std::string kMqFilterDb()
{
  static int counter = 0;
  return (std::filesystem::temp_directory_path() / ("mq_filter_test_" + std::to_string(counter++) + ".db")).string();
}

// m_configDirectory must be the first member: members initialize in declaration order, so the guard
// sets CONFIG_DIRECTORY before the constructor body builds the EnvironmentConfiguration, and it is
// destroyed last - after the test case is done with it.
struct MessageQueueFilterFixture
{
  ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_mq_filter_cfg" };
  std::string m_dbPath;
  std::shared_ptr<EnvironmentConfiguration> envConfig;
  std::shared_ptr<Configuration> configuration;
  std::shared_ptr<DatabaseConnectionConfigurations> connectionConfigurations;
  std::shared_ptr<MessageQueueRepository> repository;

  MessageQueueFilterFixture() : m_dbPath(kMqFilterDb())
  {
    std::remove(m_dbPath.c_str());

    db::Connection connection(db::ConnectionType::SQLite, m_dbPath);
    db::Statement statement(connection);
    statement.execute(R"(
            CREATE TABLE message_queues (
                name text NOT NULL,
                process_name text NOT NULL,
                max_messages integer NOT NULL,
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

  ~MessageQueueFilterFixture() { std::remove(m_dbPath.c_str()); }

  void insertQueue(const std::string &process, const std::string &queue, int32_t maxMessages = 10)
  {
    repository->insertOf({ MessageQueueEntry("test", process, queue, maxMessages) });
  }
};

std::set<std::string> queueNamesOf(const std::vector<MessageQueueEntry> &entries)
{
  std::set<std::string> names;
  for (const auto &entry : entries) { names.insert(entry.get_message_queue_name()); }
  return names;
}

}// namespace

// The three filter queries of MessageQueueRepository all bind their parameters positionally: `?` #1
// belongs to the first column of the WHERE clause and the ParameterBuilder has to add the values in
// exactly that order. A swapped pair does not raise anything - it just filters on the wrong column
// and quietly returns the wrong (often empty) row set, which is why these cases are pinned here.

TEST_CASE("MessageQueueRepository::allOf binds the queue regex to the name column")
{
  MessageQueueFilterFixture fixture;
  fixture.insertQueue("proc1", "alpha_1");
  fixture.insertQueue("proc1", "beta_1");
  fixture.insertQueue("proc2", "gamma_1");

  // The queue regex has to be evaluated against `name`, so only alpha_1 survives - although the
  // process filter `proc1` alone would also have admitted beta_1.
  const std::vector<MessageQueueEntry> entries = fixture.repository->allOf("proc1", "alpha_.*");
  REQUIRE(entries.size() == 1);
  REQUIRE(entries[0].get_message_queue_name() == "alpha_1");
  REQUIRE(entries[0].get_process_name() == "proc1");
  REQUIRE(entries[0].get_max_messages() == 10);

  // A queue regex that matches no queue at all yields an empty result, even for a process that owns
  // queues of its own.
  REQUIRE(fixture.repository->allOf("proc1", "missing_.*").empty());
}

TEST_CASE("MessageQueueRepository::allOf binds the process regex to the process_name column")
{
  MessageQueueFilterFixture fixture;
  fixture.insertQueue("proc1", "alpha_1");
  fixture.insertQueue("proc1", "beta_1");
  fixture.insertQueue("proc2", "gamma_1");

  // The process regex has to be evaluated against `process_name`, so only gamma_1 survives -
  // although the queue regex `gamma_.*` alone would also have admitted alpha_1 and beta_1.
  const std::vector<MessageQueueEntry> entries = fixture.repository->allOf("proc2", "gamma_.*");
  REQUIRE(entries.size() == 1);
  REQUIRE(entries[0].get_message_queue_name() == "gamma_1");
  REQUIRE(entries[0].get_process_name() == "proc2");

  // A process regex that matches no process at all yields an empty result, even for a queue name
  // that does exist.
  REQUIRE(fixture.repository->allOf("proc3", "gamma_.*").empty());
}

TEST_CASE("MessageQueueRepository::allOf separates a queue named like a process")
{
  MessageQueueFilterFixture fixture;
  // `proc1` exists both as a process name and as a queue name, so only the correct binding tells the
  // two rows apart.
  fixture.insertQueue("proc1", "alpha_1");
  fixture.insertQueue("proc1", "proc1");
  fixture.insertQueue("proc2", "alpha_1_backup");

  const std::vector<MessageQueueEntry> entries = fixture.repository->allOf("proc1", "alpha_1|proc1");
  REQUIRE(entries.size() == 2);
  REQUIRE(queueNamesOf(entries) == std::set<std::string>{ "alpha_1", "proc1" });
  // With the filters swapped the query becomes `name REGEXP 'proc1' and process_name REGEXP
  // 'alpha_1|proc1'` and returns the single queue named `proc1` instead of both.
  // The same holds for the unfiltered queue regex: `allOf("proc1", ".*")` owns both of proc1's
  // queues, while the swapped query collapses to the queue that happens to be named `proc1`.
  REQUIRE(queueNamesOf(fixture.repository->allOf("proc1", ".*")) == std::set<std::string>{ "alpha_1", "proc1" });
}

TEST_CASE("MessageQueueRepository::allProcessNameOf filters on the process_name column")
{
  MessageQueueFilterFixture fixture;
  fixture.insertQueue("proc1", "queue_a");
  fixture.insertQueue("proc1", "queue_b");
  fixture.insertQueue("proc2", "queue_c");

  // The predicate is `process_name = ?`. Filtering on `name` instead would find nothing at all,
  // because no queue is called `proc1`.
  REQUIRE(queueNamesOf(fixture.repository->allProcessNameOf("proc1")) == std::set<std::string>{ "queue_a", "queue_b" });
  REQUIRE(queueNamesOf(fixture.repository->allProcessNameOf("proc2")) == std::set<std::string>{ "queue_c" });

  // The comparison is exact, not a regex: `proc.*` must not select the queues of proc1.
  REQUIRE(fixture.repository->allProcessNameOf("proc.*").empty());
}

TEST_CASE("MessageQueueRepository::allMessageQueueNameOf matches the queue name regex")
{
  MessageQueueFilterFixture fixture;
  fixture.insertQueue("proc1", "alpha_1");
  fixture.insertQueue("proc2", "beta_1");

  const MessageQueueEntry entry = fixture.repository->allMessageQueueNameOf("alpha_.*");
  REQUIRE(entry.get_message_queue_name() == "alpha_1");
  REQUIRE(entry.get_process_name() == "proc1");
  REQUIRE(entry.get_max_messages() == 10);

  // `proc1` is a process name and not a queue name. A query that filtered on `process_name` would
  // wrongly hand out alpha_1 here instead of reporting that nothing matched.
  REQUIRE_THROWS_AS(fixture.repository->allMessageQueueNameOf("proc1"), db::SQLException);
  REQUIRE_THROWS_AS(fixture.repository->allMessageQueueNameOf("nothing_.*"), db::SQLException);
}