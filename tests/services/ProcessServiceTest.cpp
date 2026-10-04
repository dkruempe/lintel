#include <base_library/core/services/ProcessService.h>
#include <base_library/core/utils/ProcessResourceReader.h>
#include <base_library/features/base/configuration/Configuration.h>
#include <base_library/features/base/configuration/EnvironmentConfiguration.h>
#include <base_library/features/base/controller/ProcessGroupDto.h>
#include <base_library/features/base/controller/ProcessInfoDto.h>
#include <base_library/features/base/models/Process.h>
#include <base_library/features/base/models/ProcessGroup.h>
#include <base_library/features/base/models/ProcessInfo.h>
#include <base_library/features/base/models/ProcessName.h>
#include <base_library/features/base/services/NoopHistoryService.h>

#include <boost/process/v1.hpp>
#include <boost/process/v1/search_path.hpp>
#include <catch2/catch_all.hpp>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#include "../helpers/ScopedEnvironmentVariable.h"

#ifdef BASE_TEST_SLEEPER_BINARY
#define kHasSleeperPath true
#else
#define kHasSleeperPath false
#endif

namespace {

/** Locate a program that sleeps for a given number of seconds.
 *
 * The helper built next to the test binary (target `base_test_sleeper`, see tests/CMakeLists.txt)
 * wins: CMake passes its exact path in, so there is nothing to find at run time. Only if that file
 * is missing does this fall back to a sibling in the current working directory and finally to the
 * system `sleep` via PATH.
 *
 * @return path of the sleeper, or an empty string if no usable binary exists */
std::string findSleepBinary()
{
  if constexpr (kHasSleeperPath) {
    const std::filesystem::path helper{ BASE_TEST_SLEEPER_BINARY };
    std::error_code error;
    if (std::filesystem::is_regular_file(helper, error)) { return helper.string(); }
  }
  std::error_code error;
  const auto workingDirectory = std::filesystem::current_path(error);
  if (!error) {
    const auto candidate = workingDirectory / "base_test_sleeper";
    if (std::filesystem::is_regular_file(candidate, error)) { return candidate.string(); }
  }
  const auto systemSleep = boost::process::v1::search_path("sleep");
  if (!systemSleep.empty()) { return systemSleep.string(); }
  return {};
}

// m_configDirectory must be the first member: members initialize in declaration order, so the guard
// sets CONFIG_DIRECTORY before the constructor body builds the EnvironmentConfiguration, and it is
// destroyed last - after the test case is done with it.
struct ProcessServiceFixture
{
  ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_ps_test_cfg" };
  std::shared_ptr<ProcessName> processName;
  std::shared_ptr<EnvironmentConfiguration> envConfig;
  std::shared_ptr<Configuration> configuration;
  std::shared_ptr<NoopHistoryService> historyService;
  std::string sleepBinary;

  ProcessServiceFixture()
  {
    sleepBinary = findSleepBinary();
    if (sleepBinary.empty()) {
      // Not a SKIP: the helper binary is built together with the tests, so a missing one means the
      // build tree is broken. A SKIP here would turn a broken build into 16 test cases that quietly
      // stop testing anything - the exact silent hole this replaced.
      FAIL("no sleeper binary found - expected base_test_sleeper (target) or a system 'sleep' in PATH");
    }
    processName = std::make_shared<ProcessName>(sleepBinary);
    envConfig = std::make_shared<EnvironmentConfiguration>();
    configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, envConfig);
    historyService = std::make_shared<NoopHistoryService>(processName);
  }

  std::shared_ptr<ProcessService> createService()
  { return std::make_shared<ProcessService>(processName, envConfig, configuration, historyService); }

  /** @return a sleeper child process that outlives any test using it */
  Process makeSleepProcess() const { return Process(sleepBinary, { "600" }); }
};

}// namespace

TEST_CASE("ProcessService constructor creates self process", "[process_service]")
{
  ProcessServiceFixture fixture;
  auto service = fixture.createService();

  auto self = service->currentOf();
  REQUIRE_FALSE(self.getProcess() == nullptr);
  REQUIRE(self.isRunning());
  REQUIRE(self.getProcessId() > 0);
}

TEST_CASE("ProcessService::startOf starts a process", "[process_service]")
{
  ProcessServiceFixture fixture;
  auto service = fixture.createService();

  Process process = fixture.makeSleepProcess();
  auto future = service->startOf(process);

  REQUIRE(future.has_value());
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  auto info = service->allActiveOf();
  REQUIRE(info.size() == 2);// sleep + self
  REQUIRE(info[0].isRunning());

  service->terminateOf(process);
}

TEST_CASE("ProcessService::startOf rejects duplicate process", "[process_service]")
{
  ProcessServiceFixture fixture;
  auto service = fixture.createService();

  Process process = fixture.makeSleepProcess();
  service->startOf(process);

  REQUIRE_THROWS_AS(service->startOf(process), std::runtime_error);

  service->terminateOf(process);
}

TEST_CASE("ProcessService::startOf rejects non-existent path", "[process_service]")
{
  ProcessServiceFixture fixture;
  auto service = fixture.createService();

  Process process("/nonexistent/binary", {});
  REQUIRE_THROWS_AS(service->startOf(process), std::runtime_error);
}

TEST_CASE("ProcessService::stopOf sends signal and stops process", "[process_service]")
{
  ProcessServiceFixture fixture;
  auto service = fixture.createService();

  Process process = fixture.makeSleepProcess();
  service->startOf(process);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  bool stopped = service->stopOf(process);
  REQUIRE(stopped);

  auto info = service->allActiveOf();
  REQUIRE(info.size() == 1);// only self
}

TEST_CASE("ProcessService::terminateOf force-kills process", "[process_service]")
{
  ProcessServiceFixture fixture;
  auto service = fixture.createService();

  Process process = fixture.makeSleepProcess();
  service->startOf(process);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  service->terminateOf(process);

  auto info = service->allActiveOf();
  REQUIRE(info.size() == 1);// only self
}

TEST_CASE("ProcessService::detachOf detaches process", "[process_service]")
{
  ProcessServiceFixture fixture;
  auto service = fixture.createService();

  Process process = fixture.makeSleepProcess();
  service->startOf(process);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  service->detachOf(process);

  auto found = service->of(process.getId());
  REQUIRE_FALSE(found.has_value());
}

TEST_CASE("ProcessService::restartOf restarts process", "[process_service]")
{
  ProcessServiceFixture fixture;
  auto service = fixture.createService();

  Process process = fixture.makeSleepProcess();
  service->startOf(process);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  service->restartOf(process);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  auto info = service->allActiveOf();
  REQUIRE(info.size() == 2);// sleep + self
  REQUIRE(info[0].isRunning());

  service->terminateOf(process);
}

TEST_CASE("ProcessService::of returns process by ID", "[process_service]")
{
  ProcessServiceFixture fixture;
  auto service = fixture.createService();

  Process process = fixture.makeSleepProcess();
  service->startOf(process);

  auto found = service->of(process.getId());
  REQUIRE(found.has_value());
  REQUIRE(found.value()->getId() == process.getId());

  service->terminateOf(process);
}

TEST_CASE("ProcessService::of returns nullopt for unknown ID", "[process_service]")
{
  ProcessServiceFixture fixture;
  auto service = fixture.createService();

  auto found = service->of("nonexistent-id");
  REQUIRE_FALSE(found.has_value());
}

TEST_CASE("ProcessService::allActiveOf includes current process", "[process_service]")
{
  ProcessServiceFixture fixture;
  auto service = fixture.createService();

  auto info = service->allActiveOf();
  REQUIRE(info.size() == 1);
  REQUIRE(info[0].getProcess() == service->currentOf().getProcess());
}

TEST_CASE("ProcessService::allActiveOf refreshes resources for running process", "[process_service]")
{
  ProcessServiceFixture fixture;
  auto service = fixture.createService();

  Process process = fixture.makeSleepProcess();
  service->startOf(process);
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  bool foundValidResource = false;
  ProcessInfoDto roundTripped;
  for (const auto &info : service->allActiveOf()) {
    if (info.getProcess() == service->currentOf().getProcess()) { continue; }
    const auto &resources = info.getResourceData();
    if (resources.has_value() && resources->valid) {
      foundValidResource = true;
      ProcessInfoDto dto(info);
      REQUIRE(dto.isResourceDataValid());
      roundTripped.JsonSerializable::deserialize(dto.JsonSerializable::serialize());
    }
  }
  REQUIRE(foundValidResource);
  REQUIRE(roundTripped.isResourceDataValid());
  REQUIRE(roundTripped.getMemoryBytes().has_value());

  service->terminateOf(process);
}

TEST_CASE("ProcessService::isLastProcess", "[process_service]")
{
  ProcessServiceFixture fixture;
  auto service = fixture.createService();

  REQUIRE_FALSE(service->isLastProcess());

  Process process = fixture.makeSleepProcess();
  service->startOf(process);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  REQUIRE(service->isLastProcess());

  service->terminateOf(process);
}

TEST_CASE("ProcessService::allGroupsOf with regex", "[process_service]")
{
  ProcessServiceFixture fixture;
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

TEST_CASE("ProcessService::allGroupsOf with regex no match", "[process_service]")
{
  ProcessServiceFixture fixture;
  auto service = fixture.createService();

  Process p1 = fixture.makeSleepProcess();
  ProcessGroup group("test_group", { p1 });
  service->startOf(group);

  std::string pattern("other.*");
  auto groups = service->allGroupsOf(pattern);
  REQUIRE(groups.empty());

  service->terminateOf(group);
}

TEST_CASE("ProcessService process group lifecycle", "[process_service]")
{
  ProcessServiceFixture fixture;
  auto service = fixture.createService();

  Process p1 = fixture.makeSleepProcess();
  Process p2 = fixture.makeSleepProcess();
  ProcessGroup group("lifecycle_group", { p1, p2 });

  service->startOf(group);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  auto info = service->allActiveOf();
  REQUIRE(info.size() == 3);// 2 sleep + self

  bool stopped = service->stopOf(group);
  REQUIRE(stopped);

  info = service->allActiveOf();
  REQUIRE(info.size() == 1);// only self
}

TEST_CASE("ProcessService process group detach", "[process_service]")
{
  ProcessServiceFixture fixture;
  auto service = fixture.createService();

  Process p1 = fixture.makeSleepProcess();
  ProcessGroup group("detach_group", { p1 });

  service->startOf(group);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  service->detachOf(group);

  auto groups = service->allGroupsOf("detach_group");
  REQUIRE(groups.empty());
}

TEST_CASE("Process active window logic", "[process_service]")
{
  Process process("/bin/true", {});
  REQUIRE_FALSE(process.hasActiveWindow());
  REQUIRE(process.inActiveWindow());// no window => always active

  process.setActiveFromHour(0);
  process.setActiveToHour(24);
  REQUIRE(process.hasActiveWindow());
  REQUIRE(process.inActiveWindow());// 0..24 covers every hour

  process.setActiveFromHour(0);
  process.setActiveToHour(0);
  REQUIRE(process.hasActiveWindow());
  REQUIRE_FALSE(process.inActiveWindow());// empty window => never active
}

TEST_CASE("Process restart notification triggers", "[process_service]")
{
  Process process("/bin/true", {});
  REQUIRE_FALSE(process.getCpuNotify().has_value());
  REQUIRE_FALSE(process.getMemoryNotify().has_value());

  process.setCpuNotify(80.0);
  process.setMemoryNotify(1024ULL * 1024);
  REQUIRE(process.getCpuNotify().has_value());
  REQUIRE(process.getCpuNotify().value() == 80.0);
  REQUIRE(process.getMemoryNotify().has_value());
  REQUIRE(process.getMemoryNotify().value() == 1024ULL * 1024);
}

TEST_CASE("ProcessService::resetOf resets restarts and failure state", "[process_service]")
{
  ProcessServiceFixture fixture;
  auto service = fixture.createService();

  Process process = fixture.makeSleepProcess();
  service->startOf(process);

  auto found = service->of(process.getId());
  REQUIRE(found.has_value());
  found.value()->increaseRestarts();
  found.value()->increaseRestarts();
  REQUIRE(found.value()->currentRestarts() == 2);

  service->resetOf(process);
  auto after = service->of(process.getId());
  REQUIRE(after.has_value());
  REQUIRE(after.value()->currentRestarts() == 0);

  service->terminateOf(process);
}

// Hidden tag [linux]: the body only has an implementation on Linux, so the case has to be skipped
// elsewhere. The tag makes that selectable - `./bin/base_tests "[linux]"` runs only this case, and a
// platform gate can exclude it explicitly instead of relying on a silent skip.
TEST_CASE("ProcessResourceReader reads own process resources", "[process_service][linux]")
{
#if defined(__linux__)
  auto pid = boost::this_process::get_id();
  auto data = ProcessResourceReader::readOf(pid, std::nullopt);
  REQUIRE(data.valid);
  REQUIRE(data.uptime.count() >= 0);
  // without a previous sample the CPU percent is left unset
  if (data.cpuPercent.has_value()) { REQUIRE(data.cpuPercent.value() >= 0.0); }
#else
  SKIP("ProcessResourceReader only fully supported on Linux here");
#endif
}
