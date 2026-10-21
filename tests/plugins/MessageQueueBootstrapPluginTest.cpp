#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lintel/app/plugins/MessageQueueBootstrapPlugin.h"
#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/DatabaseConnectionComponent.h"
#include "lintel/core/configuration/DatabaseConnectionEntry.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/models/BootstrapSequence.h"
#include "lintel/core/models/ProcessName.h"
#include "lintel/core/persistence/ConnectionType.h"
#include "lintel/core/persistence/DatabaseConnectionConfigurations.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/configuration/MessageQueueComponent.h"
#include "lintel/features/base/configuration/MessageQueueEntry.h"
#include "lintel/features/base/models/Page.h"
#include "lintel/features/base/repositories/IMessageQueueRepository.h"

#include <catch2/catch_all.hpp>

/**
 * MessageQueueBootstrapPlugin reconciles the message_queues table with the MessageQueueComponent
 * configuration of the running process.
 *
 * There is no version mismatch handling in this plugin: an entry is treated as "unchanged" when the
 * queue name and max_messages match, and a changed max_messages is expressed as delete + insert of
 * the same queue name. That is what the "changed max_messages" test pins.
 */

namespace lintel {
namespace {

  /** Repository stub that records what the plugin asked it to do. */
  class MqBootstrapRepositoryStub : public IMessageQueueRepository
  {
  public:
    /** rows the stub reports for allProcessNameOf() */
    std::vector<MessageQueueEntry> existing;
    /** entries passed to insertOf() */
    std::vector<MessageQueueEntry> inserted;
    /** entries passed to deleteOf() */
    std::vector<MessageQueueEntry> deleted;
    /** number of allProcessNameOf() calls */
    int allProcessNameOfCalls = 0;
    /** process name of the last allProcessNameOf() call */
    std::string lastProcessName;

    MessageQueueEntry allMessageQueueNameOf(const std::string &) override
    {
      return MessageQueueEntry(type_name<MessageQueueComponent>(), "", "", 0);
    }

    std::vector<MessageQueueEntry> allOf(const MessageQueueFilter &) override { return {}; }

    Page<MessageQueueEntry> pageOf(const MessageQueueFilter &, const std::optional<std::string> &, std::size_t) override
    {
      return Page<MessageQueueEntry>({}, false, std::nullopt);
    }

    std::vector<MessageQueueEntry> allProcessNameOf(const std::string &processName) override
    {
      ++allProcessNameOfCalls;
      lastProcessName = processName;
      return existing;
    }

    void insertOf(const std::vector<MessageQueueEntry> &entries) override
    {
      // MessageQueueEntry has const members, so it is copyable but not assignable
      for (const auto &entry : entries) { inserted.push_back(entry); }
    }

    void deleteOf(const std::vector<MessageQueueEntry> &entries) override
    {
      for (const auto &entry : entries) { deleted.push_back(entry); }
    }

    void registerOwner(const MessageQueueOwnerRegistration &) override {}
    void updateHeartbeat(const std::string &, const std::chrono::system_clock::time_point &) override {}
    void updateOwnerState(const MessageQueueOwnerStateUpdate &) override {}
    std::vector<MessageQueueEntry> findOrphans(const std::chrono::seconds &staleThreshold) override
    {
      ++findOrphansCalls;
      lastStaleThreshold = staleThreshold;
      return orphans;
    }
    void reclaimOrphans(const std::vector<MessageQueueEntry> &entries) override
    {
      for (const auto &entry : entries) { reclaimed.push_back(entry); }
    }
    void clearOwner(const std::string &) override {}

    /** orphans the stub reports for findOrphans() */
    std::vector<MessageQueueEntry> orphans;
    /** entries passed to reclaimOrphans() */
    std::vector<MessageQueueEntry> reclaimed;
    /** number of findOrphans() calls */
    int findOrphansCalls = 0;
    /** stale threshold of the last findOrphans() call */
    std::chrono::seconds lastStaleThreshold{ 0 };

    /** rows the next allProcessNameOf() call shall report
     * @param rows the rows of the message_queues table of the current process */
    void setRows(const std::vector<MessageQueueEntry> &rows)
    {
      for (const auto &row : rows) { existing.push_back(row); }
    }
  };

  std::shared_ptr<MessageQueueEntry>
    mqBootstrapQueue(const std::string &process, const std::string &queue, int32_t maxMessages)
  {
    return std::make_shared<MessageQueueEntry>(type_name<MessageQueueComponent>(), process, queue, maxMessages);
  }

  /** @return the queue names of the given entries, in the order they were passed */
  std::vector<std::string> mqBootstrapNames(const std::vector<MessageQueueEntry> &entries)
  {
    std::vector<std::string> names;
    for (const auto &entry : entries) { names.push_back(entry.get_message_queue_name()); }
    return names;
  }

  struct MqBootstrapFixture
  {
    std::shared_ptr<Configuration> configuration;
    std::shared_ptr<DatabaseConnectionConfigurations> connectionConfigurations;
    std::shared_ptr<MqBootstrapRepositoryStub> repository;
    std::shared_ptr<ProcessName> processName;
    std::shared_ptr<MessageQueueBootstrapPlugin> plugin;

    explicit MqBootstrapFixture(const std::string &process = "main",
      bool withDefaultConnection = true,
      const std::vector<std::shared_ptr<MessageQueueEntry>> &queues = {})
    {
      std::vector<std::shared_ptr<Entry>> entries;
      if (withDefaultConnection) {
        entries.push_back(std::make_shared<DatabaseConnectionEntry>(type_name<DatabaseConnectionComponent>(),
          ":memory:",
          "",
          "",
          db::ConnectionType::SQLite,
          "default",
          -1,
          "",
          true));
      } else {
        // an entry exists, but none of them is marked as the default connection
        entries.push_back(std::make_shared<DatabaseConnectionEntry>(type_name<DatabaseConnectionComponent>(),
          ":memory:",
          "",
          "",
          db::ConnectionType::SQLite,
          "not_default",
          -1,
          "",
          false));
      }
      for (const auto &queue : queues) { entries.push_back(queue); }
      auto envConfig = std::make_shared<EnvironmentConfiguration>();
      envConfig->overrides(EnvironmentConfiguration::ConfigDirectory, "/tmp/nonexistent_mq_bootstrap_cfg");
      envConfig->overrides(EnvironmentConfiguration::BootstrapConfigName, "nonexistent");
      configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, envConfig);
      configuration->setEntries(entries);
      connectionConfigurations = std::make_shared<DatabaseConnectionConfigurations>(configuration);
      repository = std::make_shared<MqBootstrapRepositoryStub>();
      processName = std::make_shared<ProcessName>(process);
      plugin =
        std::make_shared<MessageQueueBootstrapPlugin>(connectionConfigurations, configuration, repository, processName);
    }
  };

}// namespace

TEST_CASE("MessageQueueBootstrapPlugin: getPriority is MessageQueue", "[mq_bootstrap_plugin]")
{
  MqBootstrapFixture fixture;

  REQUIRE(fixture.plugin->getPriority() == BootstrapSequence::MessageQueue);
}

TEST_CASE("MessageQueueBootstrapPlugin: without a default connection the repository stays untouched",
  "[mq_bootstrap_plugin]")
{
  MqBootstrapFixture fixture("main", false, { mqBootstrapQueue("main", "history", 10) });

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE(fixture.repository->allProcessNameOfCalls == 0);
  REQUIRE(fixture.repository->inserted.empty());
  REQUIRE(fixture.repository->deleted.empty());
}

TEST_CASE("MessageQueueBootstrapPlugin: configured queues of this process are inserted", "[mq_bootstrap_plugin]")
{
  MqBootstrapFixture fixture(
    "main", true, { mqBootstrapQueue("main", "history", 10), mqBootstrapQueue("main", "scheduler", 42) });

  fixture.plugin->onStart();

  REQUIRE(fixture.repository->allProcessNameOfCalls == 1);
  REQUIRE(fixture.repository->lastProcessName == "main");
  REQUIRE(mqBootstrapNames(fixture.repository->inserted) == std::vector<std::string>{ "history", "scheduler" });
  REQUIRE(fixture.repository->inserted[1].get_max_messages() == 42);
  REQUIRE(fixture.repository->inserted[1].get_process_name() == "main");
  REQUIRE(fixture.repository->deleted.empty());
}

TEST_CASE("MessageQueueBootstrapPlugin: queues of other processes are ignored", "[mq_bootstrap_plugin]")
{
  MqBootstrapFixture fixture("worker",
    true,
    { mqBootstrapQueue("worker", "history", 10),
      mqBootstrapQueue("main", "history", 10),
      mqBootstrapQueue("scheduler", "cron", 5) });

  fixture.plugin->onStart();

  REQUIRE(mqBootstrapNames(fixture.repository->inserted) == std::vector<std::string>{ "history" });
  REQUIRE(fixture.repository->inserted[0].get_process_name() == "worker");
  REQUIRE(fixture.repository->deleted.empty());
}

TEST_CASE("MessageQueueBootstrapPlugin: without configuration and without rows nothing changes",
  "[mq_bootstrap_plugin]")
{
  MqBootstrapFixture fixture;

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE(fixture.repository->inserted.empty());
  REQUIRE(fixture.repository->deleted.empty());
}

TEST_CASE("MessageQueueBootstrapPlugin: a queue that is no longer configured is deleted", "[mq_bootstrap_plugin]")
{
  // nothing is configured at all, so every row of the table is obsolete
  MqBootstrapFixture fixture;
  fixture.repository->setRows({ MessageQueueEntry(type_name<MessageQueueComponent>(), "main", "obsolete", 7) });

  fixture.plugin->onStart();

  REQUIRE(mqBootstrapNames(fixture.repository->deleted) == std::vector<std::string>{ "obsolete" });
  REQUIRE(fixture.repository->deleted[0].get_max_messages() == 7);
  REQUIRE(fixture.repository->inserted.empty());
}

TEST_CASE("MessageQueueBootstrapPlugin: a changed max_messages is a delete followed by an insert",
  "[mq_bootstrap_plugin]")
{
  MqBootstrapFixture fixture("main", true, { mqBootstrapQueue("main", "history", 10) });
  fixture.repository->setRows({ MessageQueueEntry(type_name<MessageQueueComponent>(), "main", "history", 5) });

  fixture.plugin->onStart();

  REQUIRE(mqBootstrapNames(fixture.repository->deleted) == std::vector<std::string>{ "history" });
  REQUIRE(fixture.repository->deleted[0].get_max_messages() == 5);
  REQUIRE(mqBootstrapNames(fixture.repository->inserted) == std::vector<std::string>{ "history" });
  // the replacement carries the configured values, not the ones read from the table
  REQUIRE(fixture.repository->inserted[0].get_max_messages() == 10);
  REQUIRE(fixture.repository->inserted[0].get_process_name() == "main");
}

TEST_CASE("MessageQueueBootstrapPlugin: an unchanged queue is neither deleted nor inserted", "[mq_bootstrap_plugin]")
{
  MqBootstrapFixture fixture("main", true, { mqBootstrapQueue("main", "history", 10) });
  fixture.repository->setRows({ MessageQueueEntry(type_name<MessageQueueComponent>(), "main", "history", 10) });

  REQUIRE_NOTHROW(fixture.plugin->onStart());

  REQUIRE(fixture.repository->inserted.empty());
  REQUIRE(fixture.repository->deleted.empty());
}

TEST_CASE("MessageQueueBootstrapPlugin: a second bootstrap does not touch an unchanged queue", "[mq_bootstrap_plugin]")
{
  MqBootstrapFixture fixture("main", true, { mqBootstrapQueue("main", "history", 10) });
  fixture.repository->setRows({ MessageQueueEntry(type_name<MessageQueueComponent>(), "main", "history", 10) });

  fixture.plugin->onStart();
  fixture.plugin->onStart();

  REQUIRE(fixture.repository->allProcessNameOfCalls == 2);
  REQUIRE(fixture.repository->inserted.empty());
  REQUIRE(fixture.repository->deleted.empty());
}

TEST_CASE("MessageQueueBootstrapPlugin: a mixed configuration inserts replaces and deletes", "[mq_bootstrap_plugin]")
{
  MqBootstrapFixture fixture("main",
    true,
    { mqBootstrapQueue("main", "unchanged", 10),
      mqBootstrapQueue("main", "fresh", 20),
      mqBootstrapQueue("main", "resized", 99) });
  fixture.repository->setRows({ MessageQueueEntry(type_name<MessageQueueComponent>(), "main", "unchanged", 10),
    MessageQueueEntry(type_name<MessageQueueComponent>(), "main", "resized", 1),
    MessageQueueEntry(type_name<MessageQueueComponent>(), "main", "gone", 3) });

  fixture.plugin->onStart();

  // "resized" is replaced while the table is read, the queues that are completely new are appended
  // afterwards, so the replacement comes first. "gone" is dropped, "unchanged" is untouched.
  REQUIRE(mqBootstrapNames(fixture.repository->inserted) == std::vector<std::string>{ "resized", "fresh" });
  REQUIRE(mqBootstrapNames(fixture.repository->deleted) == std::vector<std::string>{ "resized", "gone" });
  REQUIRE(fixture.repository->inserted[0].get_max_messages() == 99);
  REQUIRE(fixture.repository->deleted[0].get_max_messages() == 1);
}

TEST_CASE("MessageQueueBootstrapPlugin: stale owners are reclaimed before the reconciliation", "[mq_bootstrap_plugin]")
{
  MqBootstrapFixture fixture("main", true, { mqBootstrapQueue("main", "history", 10) });
  fixture.repository->orphans.push_back(MessageQueueEntry(type_name<MessageQueueComponent>(), "main", "stale", 5));

  fixture.plugin->onStart();

  REQUIRE(fixture.repository->findOrphansCalls == 1);
  REQUIRE(mqBootstrapNames(fixture.repository->reclaimed) == std::vector<std::string>{ "stale" });
}

TEST_CASE("MessageQueueBootstrapPlugin: a healthy owner is never reclaimed", "[mq_bootstrap_plugin]")
{
  MqBootstrapFixture fixture("main", true, { mqBootstrapQueue("main", "history", 10) });

  fixture.plugin->onStart();

  REQUIRE(fixture.repository->findOrphansCalls == 1);
  REQUIRE(fixture.repository->reclaimed.empty());
}

TEST_CASE("MessageQueueBootstrapPlugin: a failing orphan scan does not block the bootstrap", "[mq_bootstrap_plugin]")
{
  // the reconciliation must survive a database that cannot answer the scan
  class ThrowingOrphanRepository : public MqBootstrapRepositoryStub
  {
  public:
    std::vector<MessageQueueEntry> findOrphans(const std::chrono::seconds &) override
    {
      throw std::runtime_error("connection gone");
    }
  };
  MqBootstrapFixture fixture("main", true, { mqBootstrapQueue("main", "history", 10) });
  auto throwing = std::make_shared<ThrowingOrphanRepository>();
  auto plugin = std::make_shared<MessageQueueBootstrapPlugin>(
    fixture.connectionConfigurations, fixture.configuration, throwing, fixture.processName);

  REQUIRE_NOTHROW(plugin->onStart());
  REQUIRE(mqBootstrapNames(throwing->inserted) == std::vector<std::string>{ "history" });
}
}// namespace lintel
