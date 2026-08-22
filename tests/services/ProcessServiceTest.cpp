#include <base_library/features/base/configuration/Configuration.h>
#include <base_library/features/base/configuration/EnvironmentConfiguration.h>
#include <base_library/features/base/controller/ProcessGroupDto.h>
#include <base_library/features/base/models/Process.h>
#include <base_library/features/base/models/ProcessGroup.h>
#include <base_library/features/base/models/ProcessInfo.h>
#include <base_library/features/base/models/ProcessName.h>
#include <base_library/features/base/services/NoopHistoryService.h>
#include <base_library/core/services/ProcessService.h>

#include <catch2/catch_all.hpp>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <thread>
#include <chrono>

namespace {

struct ProcessServiceFixture {
    std::shared_ptr<ProcessName> processName;
    std::shared_ptr<EnvironmentConfiguration> envConfig;
    std::shared_ptr<Configuration> configuration;
    std::shared_ptr<NoopHistoryService> historyService;

    ProcessServiceFixture() {
        setenv("CONFIG_DIRECTORY", "/tmp/nonexistent_ps_test_cfg", 1);
        processName = std::make_shared<ProcessName>("/usr/bin/sleep");
        envConfig = std::make_shared<EnvironmentConfiguration>();
        configuration = std::make_shared<Configuration>(
                std::vector<std::shared_ptr<Component>>{}, envConfig);
        historyService = std::make_shared<NoopHistoryService>(processName);
    }

    std::shared_ptr<ProcessService> createService() {
        return std::make_shared<ProcessService>(
                processName, envConfig, configuration, historyService);
    }
};

}  // namespace

TEST_CASE("ProcessService constructor creates self process", "[process_service]") {
    ProcessServiceFixture fixture;
    auto service = fixture.createService();

    auto self = service->currentOf();
    REQUIRE_FALSE(self.getProcess() == nullptr);
    REQUIRE(self.isRunning());
    REQUIRE(self.getProcessId() > 0);
}

TEST_CASE("ProcessService::startOf starts a process", "[process_service]") {
    ProcessServiceFixture fixture;
    auto service = fixture.createService();

    Process process("/usr/bin/sleep", { "600" });
    auto future = service->startOf(process);

    REQUIRE(future.has_value());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto info = service->allActiveOf();
    REQUIRE(info.size() == 2);  // sleep + self
    REQUIRE(info[0].isRunning());

    service->terminateOf(process);
}

TEST_CASE("ProcessService::startOf rejects duplicate process", "[process_service]") {
    ProcessServiceFixture fixture;
    auto service = fixture.createService();

    Process process("/usr/bin/sleep", { "600" });
    service->startOf(process);

    REQUIRE_THROWS_AS(service->startOf(process), std::runtime_error);

    service->terminateOf(process);
}

TEST_CASE("ProcessService::startOf rejects non-existent path", "[process_service]") {
    ProcessServiceFixture fixture;
    auto service = fixture.createService();

    Process process("/nonexistent/binary", {});
    REQUIRE_THROWS_AS(service->startOf(process), std::runtime_error);
}

TEST_CASE("ProcessService::stopOf sends SIGINT and stops process", "[process_service]") {
    ProcessServiceFixture fixture;
    auto service = fixture.createService();

    Process process("/usr/bin/sleep", { "600" });
    service->startOf(process);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    bool stopped = service->stopOf(process);
    REQUIRE(stopped);

    auto info = service->allActiveOf();
    REQUIRE(info.size() == 1);  // only self
}

TEST_CASE("ProcessService::terminateOf force-kills process", "[process_service]") {
    ProcessServiceFixture fixture;
    auto service = fixture.createService();

    Process process("/usr/bin/sleep", { "600" });
    service->startOf(process);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    service->terminateOf(process);

    auto info = service->allActiveOf();
    REQUIRE(info.size() == 1);  // only self
}

TEST_CASE("ProcessService::detachOf detaches process", "[process_service]") {
    ProcessServiceFixture fixture;
    auto service = fixture.createService();

    Process process("/usr/bin/sleep", { "600" });
    service->startOf(process);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    service->detachOf(process);

    auto found = service->of(process.getId());
    REQUIRE_FALSE(found.has_value());
}

TEST_CASE("ProcessService::restartOf restarts process", "[process_service]") {
    ProcessServiceFixture fixture;
    auto service = fixture.createService();

    Process process("/usr/bin/sleep", { "600" });
    service->startOf(process);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    service->restartOf(process);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto info = service->allActiveOf();
    REQUIRE(info.size() == 2);  // sleep + self
    REQUIRE(info[0].isRunning());

    service->terminateOf(process);
}

TEST_CASE("ProcessService::of returns process by ID", "[process_service]") {
    ProcessServiceFixture fixture;
    auto service = fixture.createService();

    Process process("/usr/bin/sleep", { "600" });
    service->startOf(process);

    auto found = service->of(process.getId());
    REQUIRE(found.has_value());
    REQUIRE(found.value()->getId() == process.getId());

    service->terminateOf(process);
}

TEST_CASE("ProcessService::of returns nullopt for unknown ID", "[process_service]") {
    ProcessServiceFixture fixture;
    auto service = fixture.createService();

    auto found = service->of("nonexistent-id");
    REQUIRE_FALSE(found.has_value());
}

TEST_CASE("ProcessService::allActiveOf includes current process", "[process_service]") {
    ProcessServiceFixture fixture;
    auto service = fixture.createService();

    auto info = service->allActiveOf();
    REQUIRE(info.size() == 1);
    REQUIRE(info[0].getProcess() == service->currentOf().getProcess());
}

TEST_CASE("ProcessService::isLastProcess", "[process_service]") {
    ProcessServiceFixture fixture;
    auto service = fixture.createService();

    REQUIRE_FALSE(service->isLastProcess());

    Process process("/usr/bin/sleep", { "600" });
    service->startOf(process);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    REQUIRE(service->isLastProcess());

    service->terminateOf(process);
}

TEST_CASE("ProcessService::allGroupsOf with regex", "[process_service]") {
    ProcessServiceFixture fixture;
    auto service = fixture.createService();

    Process p1("/usr/bin/sleep", { "600" });
    Process p2("/usr/bin/sleep", { "600" });
    ProcessGroup group("test_group", { p1, p2 });

    service->startOf(group);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto groups = service->allGroupsOf("test_group");
    REQUIRE(groups.size() == 1);
    REQUIRE(groups[0].getName() == "test_group");

    service->terminateOf(group);
}

TEST_CASE("ProcessService::allGroupsOf with regex no match", "[process_service]") {
    ProcessServiceFixture fixture;
    auto service = fixture.createService();

    Process p1("/usr/bin/sleep", { "600" });
    ProcessGroup group("test_group", { p1 });
    service->startOf(group);

    auto groups = service->allGroupsOf("other.*");
    REQUIRE(groups.empty());

    service->terminateOf(group);
}

TEST_CASE("ProcessService process group lifecycle", "[process_service]") {
    ProcessServiceFixture fixture;
    auto service = fixture.createService();

    Process p1("/usr/bin/sleep", { "600" });
    Process p2("/usr/bin/sleep", { "600" });
    ProcessGroup group("lifecycle_group", { p1, p2 });

    service->startOf(group);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto info = service->allActiveOf();
    REQUIRE(info.size() == 3);  // 2 sleep + self

    bool stopped = service->stopOf(group);
    REQUIRE(stopped);

    info = service->allActiveOf();
    REQUIRE(info.size() == 1);  // only self
}

TEST_CASE("ProcessService process group detach", "[process_service]") {
    ProcessServiceFixture fixture;
    auto service = fixture.createService();

    Process p1("/usr/bin/sleep", { "600" });
    ProcessGroup group("detach_group", { p1 });

    service->startOf(group);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    service->detachOf(group);

    auto groups = service->allGroupsOf("detach_group");
    REQUIRE(groups.empty());
}
