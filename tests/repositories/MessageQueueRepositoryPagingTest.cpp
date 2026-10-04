#include <base_library/core/persistence/Connection.h>
#include <base_library/core/persistence/Statement.h>
#include <base_library/core/persistence/DatabaseConnectionConfigurations.h>
#include <base_library/features/base/configuration/Configuration.h>
#include <base_library/features/base/configuration/DatabaseConnectionComponent.h>
#include <base_library/features/base/configuration/DatabaseConnectionEntry.h>
#include <base_library/features/base/configuration/EnvironmentConfiguration.h>
#include <base_library/features/base/configuration/MessageQueueEntry.h>
#include <base_library/features/base/repositories/MessageQueueRepository.h>

#include <catch2/catch_all.hpp>

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "../helpers/ScopedEnvironmentVariable.h"

namespace {

std::string kMqPagingDb() {
    static int counter = 0;
    return (std::filesystem::temp_directory_path() /
            ("mq_paging_test_" + std::to_string(counter++) + ".db"))
            .string();
}

// m_configDirectory must be the first member: members initialize in declaration order, so the guard
// sets CONFIG_DIRECTORY before the constructor body builds the EnvironmentConfiguration, and it is
// destroyed last - after the test case is done with it.
struct MessageQueuePagingFixture {
  ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_mq_paging_cfg" };
    std::string m_dbPath;
    std::shared_ptr<EnvironmentConfiguration> envConfig;
    std::shared_ptr<Configuration> configuration;
    std::shared_ptr<DatabaseConnectionConfigurations> connectionConfigurations;
    std::shared_ptr<MessageQueueRepository> repository;

    MessageQueuePagingFixture() : m_dbPath(kMqPagingDb()) {
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
        configuration = std::make_shared<Configuration>(
                std::vector<std::shared_ptr<Component>>{}, envConfig);
        auto entry = std::make_shared<DatabaseConnectionEntry>(
                type_name<DatabaseConnectionComponent>(), m_dbPath, "", "",
                db::ConnectionType::SQLite, "default", 0, "", true);
        configuration->setEntries(std::vector<std::shared_ptr<Entry>>{entry});
        connectionConfigurations =
                std::make_shared<DatabaseConnectionConfigurations>(configuration);
        repository = std::make_shared<MessageQueueRepository>(connectionConfigurations);
    }

    ~MessageQueuePagingFixture() { std::remove(m_dbPath.c_str()); }

    void insertQueue(const std::string &process, const std::string &queue,
                     int32_t maxMessages = 10) {
        repository->insertOf({MessageQueueEntry("test", process, queue, maxMessages)});
    }
};

}  // namespace

TEST_CASE("MessageQueueRepository::pageOf walks through all queues",
          "[message_queue_paging]") {
    MessageQueuePagingFixture fixture;
    for (const char *queue : {"queue_a", "queue_b", "queue_c", "queue_d", "queue_e"}) {
        fixture.insertQueue("proc1", queue);
    }

    const Page<MessageQueueEntry> page1 =
            fixture.repository->pageOf(".*", ".*", std::nullopt, 2);
    REQUIRE(page1.getItems().size() == 2);
    REQUIRE(page1.getItems()[0].get_message_queue_name() == "queue_a");
    REQUIRE(page1.getItems()[1].get_message_queue_name() == "queue_b");
    REQUIRE(page1.hasMore());
    REQUIRE(page1.getNextAfter().has_value());
    REQUIRE(page1.getNextAfter().value() == "queue_b");

    const Page<MessageQueueEntry> page2 =
            fixture.repository->pageOf(".*", ".*", page1.getNextAfter(), 2);
    REQUIRE(page2.getItems().size() == 2);
    REQUIRE(page2.getItems()[0].get_message_queue_name() == "queue_c");
    REQUIRE(page2.getItems()[1].get_message_queue_name() == "queue_d");
    REQUIRE(page2.hasMore());

    const Page<MessageQueueEntry> page3 =
            fixture.repository->pageOf(".*", ".*", page2.getNextAfter(), 2);
    REQUIRE(page3.getItems().size() == 1);
    REQUIRE(page3.getItems()[0].get_message_queue_name() == "queue_e");
    REQUIRE_FALSE(page3.hasMore());
    REQUIRE_FALSE(page3.getNextAfter().has_value());
}

TEST_CASE("MessageQueueRepository::pageOf respects process filter",
          "[message_queue_paging]") {
    MessageQueuePagingFixture fixture;
    fixture.insertQueue("proc1", "queue_a");
    fixture.insertQueue("proc2", "queue_b");
    fixture.insertQueue("proc1", "queue_c");

    const Page<MessageQueueEntry> page =
            fixture.repository->pageOf("proc1", ".*", std::nullopt, 10);
    REQUIRE(page.getItems().size() == 2);
    REQUIRE_FALSE(page.hasMore());
}

TEST_CASE("MessageQueueRepository::pageOf respects queue name filter",
          "[message_queue_paging]") {
    MessageQueuePagingFixture fixture;
    fixture.insertQueue("proc1", "alpha_1");
    fixture.insertQueue("proc1", "beta_1");
    fixture.insertQueue("proc1", "beta_2");

    const Page<MessageQueueEntry> page =
            fixture.repository->pageOf(".*", "beta_.*", std::nullopt, 10);
    REQUIRE(page.getItems().size() == 2);
    REQUIRE(page.getItems()[0].get_message_queue_name() == "beta_1");
    REQUIRE(page.getItems()[1].get_message_queue_name() == "beta_2");
}

TEST_CASE("MessageQueueRepository::pageOf returns empty page without matches",
          "[message_queue_paging]") {
    MessageQueuePagingFixture fixture;
    fixture.insertQueue("proc1", "queue_a");

    const Page<MessageQueueEntry> page =
            fixture.repository->pageOf("unknown_proc", ".*", std::nullopt, 5);
    REQUIRE(page.getItems().empty());
    REQUIRE_FALSE(page.hasMore());
    REQUIRE_FALSE(page.getNextAfter().has_value());
}
