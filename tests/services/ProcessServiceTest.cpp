#include <base_library/features/base/configuration/Configuration.h>
#include <base_library/features/base/configuration/EnvironmentConfiguration.h>
#include <base_library/features/base/controller/ProcessGroupDto.h>
#include <base_library/features/base/models/Process.h>
#include <base_library/features/base/models/ProcessGroup.h>
#include <base_library/features/base/models/ProcessInfo.h>
#include <base_library/features/base/models/ProcessName.h>
#include <base_library/features/base/services/NoopHistoryService.h>
#include <base_library/core/services/ProcessService.h>

#include <boost/process/v1/search_path.hpp>
#include <catch2/catch_all.hpp>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

namespace {

std::string findSleepBinary() {
    auto path = boost::process::v1::search_path("sleep");
    if (!path.empty()) {
        return path.string();
    }
#if defined(_WIN32)
    auto timeoutPath = boost::process::v1::search_path("timeout");
    if (!timeoutPath.empty()) {
        return timeoutPath.string();
    }
#endif
    return {};
}

struct ProcessServiceFixture {
    std::shared_ptr<ProcessName> processName;
    std::shared_ptr<EnvironmentConfiguration> envConfig;
    std::shared_ptr<Configuration> configuration;
    std::shared_ptr<NoopHistoryService> historyService;
    std::string sleepBinary;

    ProcessServiceFixture() {
        sleepBinary = findSleepBinary();
        setenv("CONFIG_DIRECTORY", "/tmp/nonexistent_ps_test_cfg", 1);
        processName = std::make_shared<ProcessName>(sleepBinary.empty() ? "/bin/true" : sleepBinary);
        envConfig = std::make_shared<EnvironmentConfiguration>();
        configuration = std::make_shared<Configuration>(
                std::vector<std::shared_ptr<Component>>{}, envConfig);
        historyService = std::make_shared<NoopHistoryService>(processName);
    }

    std::shared_ptr<ProcessService> createService() {
        return std::make_shared<ProcessService>(
                processName, envConfig, configuration, historyService);
    }

    Process makeSleepProcess() const {
#if defined(_WIN32)
        return Process(sleepBinary, { "5" });
#else
        return Process(sleepBinary, { "600" });
#endif
    }
};

}  // namespace

TEST_CASE("ProcessService constructor creates self process", "[process_service]") {
    ProcessServiceFixture fixture;
    if (fixture.sleepBinary.empty()) { SKIP("sleep binary not found"); }
    auto service = fixture.createService();

    auto self = service->currentOf();
    REQUIRE_FALSE(self.getProcess() == nullptr);
    REQUIRE(self.isRunning());
    REQUIRE(self.getProcessId() > 0);
}

TEST_CASE("ProcessService::startOf starts a process", "[process_service]") {
    ProcessServiceFixture fixture;
    if (fixture.sleepBinary.empty()) { SKIP("sleep binary not found"); }
    auto service = fixture.createService();

    Process process = fixture.makeSleepProcess();
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
    if (fixture.sleepBinary.empty()) { SKIP("sleep binary not found"); }
    auto service = fixture.createService();

    Process process = fixture.makeSleepProcess();
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

TEST_CASE("ProcessService::stopOf sends signal and stops process", "[process_service]") {
    ProcessServiceFixture fixture;
    if (fixture.sleepBinary.empty()) { SKIP("sleep binary not found"); }
    auto service = fixture.createService();

    Process process = fixture.makeSleepProcess();
    service->startOf(process);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    bool stopped = service->stopOf(process);
    REQUIRE(stopped);

    auto info = service->allActiveOf();
    REQUIRE(info.size() == 1);  // only self
}

TEST_CASE("ProcessService::terminateOf force-kills process", "[process_service]") {
    ProcessServiceFixture fixture;
    if (fixture.sleepBinary.empty()) { SKIP("sleep binary not found"); }
    auto service = fixture.createService();

    Process process = fixture.makeSleepProcess();
    service->startOf(process);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    service->terminateOf(process);

    auto info = service->allActiveOf();
    REQUIRE(info.size() == 1);  // only self
}

TEST_CASE("ProcessService::detachOf detaches process", "[process_service]") {
    ProcessServiceFixture fixture;
    if (fixture.sleepBinary.empty()) { SKIP("sleep binary not found"); }
    auto service = fixture.createService();

    Process process = fixture.makeSleepProcess();
    service->startOf(process);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    service->detachOf(process);

    auto found = service->of(process.getId());
    REQUIRE_FALSE(found.has_value());
}

TEST_CASE("ProcessService::restartOf restarts process", "[process_service]") {
    ProcessServiceFixture fixture;
    if (fixture.sleepBinary.empty()) { SKIP("sleep binary not found"); }
    auto service = fixture.createService();

    Process process = fixture.makeSleepProcess();
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
    if (fixture.sleepBinary.empty()) { SKIP("sleep binary not found"); }
    auto service = fixture.createService();

    Process process = fixture.makeSleepProcess();
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
    if (fixture.sleepBinary.empty()) { SKIP("sleep binary not found"); }
    auto service = fixture.createService();

    auto info = service->allActiveOf();
    REQUIRE(info.size() == 1);
    REQUIRE(info[0].getProcess() == service->currentOf().getProcess());
}

TEST_CASE("ProcessService::isLastProcess", "[process_service]") {
    ProcessServiceFixture fixture;
    if (fixture.sleepBinary.empty()) { SKIP("sleep binary not found"); }
    auto service = fixture.createService();

    REQUIRE_FALSE(service->isLastProcess());

    Process process = fixture.makeSleepProcess();
    service->startOf(process);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    REQUIRE(service->isLastProcess());

    service->terminateOf(process);
}

TEST_CASE("ProcessService::allGroupsOf with regex", "[process_service]") {
    ProcessServiceFixture fixture;
    if (fixture.sleepBinary.empty()) { SKIP("sleep binary not found"); }
    auto service = fixture.createService();

    Process p1 = fixture.makeSleepProcess();
    Process p2 = fixture.makeSleepProcess();
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
    if (fixture.sleepBinary.empty()) { SKIP("sleep binary not found"); }
    auto service = fixture.createService();

    Process p1 = fixture.makeSleepProcess();
    ProcessGroup group("test_group", { p1 });
    service->startOf(group);

    std::string pattern("other.*");
    auto groups = service->allGroupsOf(pattern);
    REQUIRE(groups.empty());

    service->terminateOf(group);
}

TEST_CASE("ProcessService process group lifecycle", "[process_service]") {
    ProcessServiceFixture fixture;
    if (fixture.sleepBinary.empty()) { SKIP("sleep binary not found"); }
    auto service = fixture.createService();

    Process p1 = fixture.makeSleepProcess();
    Process p2 = fixture.makeSleepProcess();
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
    if (fixture.sleepBinary.empty()) { SKIP("sleep binary not found"); }
    auto service = fixture.createService();

    Process p1 = fixture.makeSleepProcess();
    ProcessGroup group("detach_group", { p1 });

    service->startOf(group);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    service->detachOf(group);

    auto groups = service->allGroupsOf("detach_group");
    REQUIRE(groups.empty());
}
