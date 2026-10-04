#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <unistd.h>

#include <boost/interprocess/managed_mapped_file.hpp>

#include "base_library/core/models/SharedMemorySegment.h"
#include "base_library/core/plugins/SharedMemoryBootstrapPlugin.h"
#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/core/persistence/Statement.h"
#include "base_library/core/utils/TypeName.h"
#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/configuration/DatabaseConnectionComponent.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"
#include "base_library/features/base/configuration/EventBusComponent.h"
#include "base_library/features/base/configuration/EventBusEntry.h"
#include "base_library/features/base/configuration/SharedMemorySegmentComponent.h"
#include "base_library/features/base/configuration/SharedMemorySegmentEntry.h"
#include "base_library/features/base/events/BoostSegmentAllocator.h"
#include "base_library/features/base/events/EventBus.h"
#include "base_library/features/base/repositories/SharedMemoryRepository.h"
#include "base_library/features/base/services/SharedMemorySegmentManager.h"

#include <catch2/catch_all.hpp>

#include "../helpers/ScopedEnvironmentVariable.h"

namespace {

using Segment = boost::interprocess::managed_mapped_file;

std::filesystem::path uniquePath(const std::string &tag) {
    static unsigned int counter = 0;
    const auto path = std::filesystem::temp_directory_path() /
                      ("shm_plugin_test_" + tag + "_" +
                       std::to_string(::getpid()) + "_" +
                       std::to_string(counter++) + ".bin");
    std::filesystem::remove(path);
    return path;
}

class TestRepository : public SharedMemoryRepository {
public:
    TestRepository(std::shared_ptr<SharedMemorySegment> segment, int32_t codeVersion,
                   std::string uuid)
        : SharedMemoryRepository(std::move(segment), 128, "TestRepository", codeVersion,
                                 std::move(uuid)) {}

    [[nodiscard]] SharedMemoryType getType() const override {
        return SharedMemoryType::Object;
    }
    void serialize(rapidjson::Writer<rapidjson::StringBuffer> *) const override {}
    bool deserialize(const rapidjson::Value &) override { return true; }
    void onMigrate(int32_t) override {}
};

// m_configDirectory must be the first member: members initialize in declaration order, so the
// guard sets CONFIG_DIRECTORY before the constructor body builds the EnvironmentConfiguration,
// and it is destroyed last - after the test case is done with it.
struct ShmPluginFixture {
  ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test" };
    static constexpr std::size_t propertySize = 1u << 20;
    static constexpr std::size_t eventBusSize = 8u << 20;

    std::filesystem::path propertyPath;
    std::filesystem::path eventBusPath;
    std::shared_ptr<Configuration> config;
    std::shared_ptr<TestRepository> repository;
    std::shared_ptr<SharedMemoryBootstrapPlugin> plugin;

    explicit ShmPluginFixture(bool createFiles, bool withEventBus = true,
                     const std::shared_ptr<DatabaseConnectionEntry> &dbEntry = nullptr,
                     const std::shared_ptr<EventBusEntry> &eventBusOverride = nullptr)
        : propertyPath(uniquePath("property")), eventBusPath(uniquePath("eventbus")) {
        if (createFiles) {
            Segment(boost::interprocess::open_or_create, propertyPath.c_str(), propertySize);
            Segment(boost::interprocess::open_or_create, eventBusPath.c_str(), eventBusSize);
        }
        std::vector<std::shared_ptr<Entry>> entries;
        auto propertySegment =
                std::make_shared<SharedMemorySegment>(propertyPath, "shm_property", propertySize);
        entries.push_back(std::make_shared<SharedMemorySegmentEntry>(
                type_name<SharedMemorySegmentComponent>(), propertySegment));
        auto eventBusSegment =
                std::make_shared<SharedMemorySegment>(eventBusPath, "shm_eventbus", eventBusSize);
        entries.push_back(std::make_shared<SharedMemorySegmentEntry>(
                type_name<SharedMemorySegmentComponent>(), eventBusSegment));
        if (withEventBus) {
            entries.push_back(eventBusOverride != nullptr
                                      ? eventBusOverride
                                      : std::make_shared<EventBusEntry>(
                                                type_name<EventBusComponent>(), "main",
                                                "shm_eventbus", 128, 128, 8, 8));
        }
        if (dbEntry != nullptr) {
            entries.push_back(dbEntry);
        }
        config = std::make_shared<Configuration>(
                std::vector<std::shared_ptr<Component>>{},
                std::make_shared<EnvironmentConfiguration>());
        config->setEntries(std::move(entries));
        auto segmentManager = std::make_shared<SharedMemorySegmentManager>(config);
        auto connectionConfigs = std::make_shared<DatabaseConnectionConfigurations>(config);
        repository = std::make_shared<TestRepository>(propertySegment, 1, "test-repository");
        plugin = std::make_shared<SharedMemoryBootstrapPlugin>(
                connectionConfigs,
                std::vector<std::shared_ptr<SharedMemoryRepository>>{repository}, segmentManager,
                config);
    }

    ~ShmPluginFixture() {
        std::filesystem::remove(propertyPath);
        std::filesystem::remove(eventBusPath);
    }
};

void createBusInSegment(const std::filesystem::path &path, const EventBusConfig &config,
                        const char *name = "main") {
    Segment mapping(boost::interprocess::open_only, path.c_str());
    BoostSegmentAllocator allocator(*mapping.get_segment_manager());
    mapping.find_or_construct<EventBus>(name)(name, config, ShmSegmentAccessor(allocator));
}

std::shared_ptr<DatabaseConnectionEntry> sqliteEntry(const std::filesystem::path &dbPath) {
    return std::make_shared<DatabaseConnectionEntry>(
            type_name<DatabaseConnectionComponent>(), dbPath.string(), "", "",
            db::ConnectionType::SQLite, "test_db", -1, "", true);
}

void createTable(const std::filesystem::path &dbPath) {
    db::Connection conn(db::ConnectionType::SQLite, dbPath.string());
    db::Statement stmt(conn);
    stmt.execute("CREATE TABLE shared_memory_repositories ("
                 "uuid TEXT PRIMARY KEY,"
                 "shared_memory_segment TEXT,"
                 "shared_memory_type TEXT,"
                 "shared_memory_repository TEXT,"
                 "current_version INTEGER,"
                 "current_data_size INTEGER)");
}

}  // namespace

TEST_CASE("SharedMemoryBootstrapPlugin: missing segment files are valid") {
    ShmPluginFixture fixture(false);
    REQUIRE_NOTHROW(fixture.plugin->onStart());
}

TEST_CASE("SharedMemoryBootstrapPlugin: healthy segment files are valid") {
    ShmPluginFixture fixture(true);
    REQUIRE_NOTHROW(fixture.plugin->onStart());
}

TEST_CASE("SharedMemoryBootstrapPlugin: truncated segment file throws") {
    ShmPluginFixture fixture(true);
    std::filesystem::resize_file(fixture.propertyPath, 5000);
    REQUIRE_THROWS_AS(fixture.plugin->onStart(), std::runtime_error);
}

TEST_CASE("SharedMemoryBootstrapPlugin: garbage segment file throws") {
    ShmPluginFixture fixture(true);
    std::filesystem::resize_file(fixture.propertyPath, 64);
    REQUIRE_THROWS_AS(fixture.plugin->onStart(), std::runtime_error);
}

TEST_CASE("SharedMemoryBootstrapPlugin: event bus with matching config is valid") {
    ShmPluginFixture fixture(true);
    EventBusConfig config;
    config.busCapacity = 128;
    config.subscriberCapacity = 128;
    config.maxTopics = 8;
    config.maxSubscribers = 8;
    createBusInSegment(fixture.eventBusPath, config);
    REQUIRE_NOTHROW(fixture.plugin->onStart());
}

TEST_CASE("SharedMemoryBootstrapPlugin: event bus with different config throws") {
    ShmPluginFixture fixture(true);
    EventBusConfig config;
    config.busCapacity = 64;
    config.subscriberCapacity = 128;
    config.maxTopics = 8;
    config.maxSubscribers = 8;
    createBusInSegment(fixture.eventBusPath, config);
    REQUIRE_THROWS_AS(fixture.plugin->onStart(), std::runtime_error);
}

TEST_CASE("SharedMemoryBootstrapPlugin: invalid event bus config throws") {
    auto badEntry = std::make_shared<EventBusEntry>(
            type_name<EventBusComponent>(), "main", "shm_eventbus", 128, 128, 0, 8);
    ShmPluginFixture fixture(true, true, nullptr, badEntry);
    REQUIRE_THROWS_AS(fixture.plugin->onStart(), std::runtime_error);
}

TEST_CASE("SharedMemoryBootstrapPlugin: inserts missing database bookmark") {
    const auto dbPath = uniquePath("db");
    createTable(dbPath);
    ShmPluginFixture fixture(true, false, sqliteEntry(dbPath));
    REQUIRE_NOTHROW(fixture.plugin->onStart());
    db::Connection conn(db::ConnectionType::SQLite, dbPath.string());
    db::Statement stmt(conn);
    auto result = stmt.execute("SELECT count(*) FROM shared_memory_repositories");
    REQUIRE(result.getValue(0, 0) == "1");
    std::filesystem::remove(dbPath);
}

TEST_CASE("SharedMemoryBootstrapPlugin: matching database bookmark is valid") {
    const auto dbPath = uniquePath("db");
    createTable(dbPath);
    {
        db::Connection conn(db::ConnectionType::SQLite, dbPath.string());
        db::Statement stmt(conn);
        stmt.execute("INSERT INTO shared_memory_repositories VALUES "
                     "('test-repository', 'shm_property', 'Object', 'TestRepository', 1, 128)");
    }
    ShmPluginFixture fixture(true, false, sqliteEntry(dbPath));
    REQUIRE_NOTHROW(fixture.plugin->onStart());
    std::filesystem::remove(dbPath);
}

TEST_CASE("SharedMemoryBootstrapPlugin: database version mismatch throws") {
    const auto dbPath = uniquePath("db");
    createTable(dbPath);
    {
        db::Connection conn(db::ConnectionType::SQLite, dbPath.string());
        db::Statement stmt(conn);
        stmt.execute("INSERT INTO shared_memory_repositories VALUES "
                     "('test-repository', 'shm_property', 'Object', 'TestRepository', 99, 128)");
    }
    ShmPluginFixture fixture(true, false, sqliteEntry(dbPath));
    REQUIRE_THROWS_AS(fixture.plugin->onStart(), std::runtime_error);
    std::filesystem::remove(dbPath);
}
