#include <cstdlib>
#include <filesystem>
#include <memory>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "base_library/core/models/SharedMemorySegment.h"
#include "base_library/core/persistence/ConnectionType.h"
#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/core/plugins/SingleInstanceBootstrapPlugin.h"
#include "base_library/core/utils/TypeName.h"
#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/configuration/DatabaseConnectionComponent.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"
#include "base_library/features/base/configuration/EnvironmentConfiguration.h"
#include "base_library/features/base/configuration/SharedMemorySegmentComponent.h"
#include "base_library/features/base/configuration/SharedMemorySegmentEntry.h"
#include "base_library/features/base/models/ProcessName.h"

#include <catch2/catch_all.hpp>

namespace {

std::filesystem::path uniqueLockDir(const std::string &tag) {
    static unsigned int counter = 0;
    const auto path = std::filesystem::temp_directory_path() /
                      ("single_instance_test_" + tag + "_" +
                       std::to_string(::getpid()) + "_" +
                       std::to_string(counter++));
    std::filesystem::remove_all(path);
    return path;
}

struct SingleInstanceFixture {
    std::filesystem::path lockDir;
    std::shared_ptr<Configuration> config;
    std::shared_ptr<ProcessName> processName;
    std::shared_ptr<DatabaseConnectionConfigurations> connectionConfigs;

    explicit SingleInstanceFixture(const std::string &process,
                     const std::string &tag = "default",
                     bool withDatabase = true,
                     const std::filesystem::path &dir = {})
        : lockDir(dir.empty() ? uniqueLockDir(tag) : dir) {
        setenv("CONFIG_DIRECTORY", "/tmp/nonexistent_cfg_test", 1);
        std::vector<std::shared_ptr<Entry>> entries;
        entries.push_back(std::make_shared<SharedMemorySegmentEntry>(
                type_name<SharedMemorySegmentComponent>(),
                std::make_shared<std::filesystem::path>(lockDir)));
        if (withDatabase) {
            entries.push_back(std::make_shared<DatabaseConnectionEntry>(
                    type_name<DatabaseConnectionComponent>(), ":memory:", "", "",
                    db::ConnectionType::SQLite, "test_db", -1, "", true));
        }
        config = std::make_shared<Configuration>(
                std::vector<std::shared_ptr<Component>>{},
                std::make_shared<EnvironmentConfiguration>());
        config->setEntries(std::move(entries));
        processName = std::make_shared<ProcessName>(process);
        connectionConfigs = std::make_shared<DatabaseConnectionConfigurations>(config);
    }

    ~SingleInstanceFixture() {
        std::filesystem::remove_all(lockDir);
    }
};

std::shared_ptr<SingleInstanceBootstrapPlugin> pluginOf(const SingleInstanceFixture &fixture) {
    return std::make_shared<SingleInstanceBootstrapPlugin>(
            fixture.connectionConfigs, fixture.config, fixture.processName);
}

/** Run pluginOf(fixture)->onStart() in a forked child process.
 * @param fixture the test fixture
 * @param expectThrow whether the child must observe the single instance conflict
 * @return 0 if the child behaved as expected, non-zero otherwise */
int runInChild(const SingleInstanceFixture &fixture, bool expectThrow) {
    int pipeFds[2];
    REQUIRE(::pipe(pipeFds) == 0);
    const pid_t pid = ::fork();
    if (pid == 0) {
        ::close(pipeFds[0]);
        bool threw = false;
        try {
            auto plugin = pluginOf(fixture);
            plugin->onStart();
        } catch (const std::runtime_error &) {
            threw = true;
        }
        const int result = (threw == expectThrow) ? 0 : 1;
        const ssize_t written = ::write(pipeFds[1], &result, sizeof(result));
        ::close(pipeFds[1]);
        _exit(written == static_cast<ssize_t>(sizeof(result)) ? 0 : 1);
    }
    ::close(pipeFds[1]);
    int childResult = 1;
    REQUIRE(::read(pipeFds[0], &childResult, sizeof(childResult)) ==
            static_cast<ssize_t>(sizeof(childResult)));
    ::close(pipeFds[0]);
    int status = 0;
    REQUIRE(::waitpid(pid, &status, 0) == pid);
    return childResult;
}

}  // namespace

TEST_CASE("SingleInstanceBootstrapPlugin: no default database skips the lock") {
    SingleInstanceFixture fixture("main", "no_db", false);
    auto plugin = pluginOf(fixture);
    REQUIRE_NOTHROW(plugin->onStart());
    REQUIRE_FALSE(std::filesystem::exists(fixture.lockDir / "main.lock"));
}

TEST_CASE("SingleInstanceBootstrapPlugin: first instance acquires the lock") {
    SingleInstanceFixture fixture("main");
    auto plugin = pluginOf(fixture);
    REQUIRE_NOTHROW(plugin->onStart());
    REQUIRE(std::filesystem::exists(fixture.lockDir / "main.lock"));
}

TEST_CASE("SingleInstanceBootstrapPlugin: a second process of the same name throws") {
    SingleInstanceFixture fixture("main");
    auto first = pluginOf(fixture);
    REQUIRE_NOTHROW(first->onStart());
    REQUIRE(runInChild(fixture, true) == 0);
}

TEST_CASE("SingleInstanceBootstrapPlugin: lock is released when the instance ends") {
    SingleInstanceFixture fixture("main");
    auto first = pluginOf(fixture);
    REQUIRE_NOTHROW(first->onStart());
    first.reset();
    REQUIRE(runInChild(fixture, false) == 0);
}

TEST_CASE("SingleInstanceBootstrapPlugin: different process names are independent") {
    SingleInstanceFixture fixture("main", "multi");
    SingleInstanceFixture other("worker", "multi_other");
    auto main = pluginOf(fixture);
    auto worker = pluginOf(other);
    REQUIRE_NOTHROW(main->onStart());
    REQUIRE_NOTHROW(worker->onStart());
    REQUIRE(std::filesystem::exists(fixture.lockDir / "main.lock"));
    REQUIRE(std::filesystem::exists(other.lockDir / "worker.lock"));
}

