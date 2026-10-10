#include <httplib.h>
#include <rapidjson/document.h>

#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include <lintel/core/services/ProcessService.h>
#include <lintel/features/base/configuration/Configuration.h>
#include <lintel/features/base/configuration/EnvironmentConfiguration.h>
#include <lintel/features/base/controller/ProcessGroupsDto.h>
#include <lintel/features/base/controller/ProcessInfoDto.h>
#include <lintel/features/base/controller/ProcessInfosDto.h>
#include <lintel/features/base/models/Group.h>
#include <lintel/features/base/models/ProcessName.h>
#include <lintel/features/base/models/User.h>
#include <lintel/features/base/services/NoopHistoryService.h>
#include <lintel/features/http/controllers/ProcessController.h>
#include <lintel/features/http/service/ContentType.h>
#include <lintel/features/http/service/HttpStatusCodes.h>


#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <unistd.h>

#include "../helpers/ScopedEnvironmentVariable.h"
#include "../mocks/MockAuthService.h"

using namespace trompeloeil;

namespace {

// `pc` prefix: unity builds merge several test files into one translation unit
// and share one anonymous namespace, so a generic `adminGroup` or `fixture`
// would collide (and shadow local variables, Clang -Wshadow).
constexpr std::chrono::milliseconds pcStartupTimeout{ 5000 };
constexpr char pcTokenAdmin[] = "process-token-admin";
constexpr char pcTokenPlain[] = "process-token-plain";
constexpr char pcTokenOutsider[] = "process-token-outsider";
constexpr char pcUnknownProcessId[] = "pc-process-that-does-not-exist";

#ifdef BASE_TEST_SLEEPER_BINARY
constexpr bool kPcHasSleeperPath = true;
#else
constexpr bool kPcHasSleeperPath = false;
#endif

Group pcAdminGroup{ "Admin-Process", {}, true };
Group pcUserGroup{ "User-Process", {}, true };
Group pcUnrelatedGroup{ "Some-Other-Group", {}, true };

User pcAdminUser("Proc", "Admin", User::Female, "admin@pc.test", "pcAdmin", "", { pcAdminGroup });
User pcPlainUser("Proc", "Plain", User::Male, "plain@pc.test", "pcPlain", "", { pcUserGroup });
User pcOutsiderUser("Proc", "Outsider", User::Male, "out@pc.test", "pcOutsider", "", { pcUnrelatedGroup });

UserToken pcTokenOf(const User &user, const std::string &id)
{
  return UserToken{
    "127.0.0.1", id, std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()), user
  };
}

/** @return the Authorization header that carries the given bearer token */
httplib::Headers pcAuthHeaders(const char *token)
{
  return httplib::Headers{ { "Authorization", std::string("Bearer ") + token } };
}

/**
 * @return Authorization plus an explicit Content-Type
 *
 * The ADD_HANDLER_METHOD wrapper only defaults Content-Type to application/json
 * for GET. Every other method reads the header exactly as sent, so even a DELETE
 * without a body has to declare application/json - otherwise the controller
 * answers 403 ("wrong content type") before it ever looks at the process id.
 */
httplib::Headers pcAuthJsonHeaders(const char *token)
{
  return httplib::Headers{ { "Authorization", std::string("Bearer ") + token },
    { "Content-Type", "application/json" } };
}

/**
 * Locate the stand-in for the system `sleep` utility that is built next to the
 * test binary (target `base_test_sleeper`). CMake passes its exact path in, so
 * there is nothing to look up at run time; the current directory and the system
 * `sleep` are only fallbacks.
 * @return path of the sleeper, or an empty string if nothing usable exists */
std::string pcFindSleeperBinary()
{
  if constexpr (kPcHasSleeperPath) {
    const std::filesystem::path helper{ BASE_TEST_SLEEPER_BINARY };
    std::error_code error;
    if (std::filesystem::is_regular_file(helper, error)) { return helper.string(); }
  }
  std::error_code error;
  const auto workingDirectory = std::filesystem::current_path(error);
  if (!error) {
    const auto candidate = workingDirectory / "base_test_sleeper";
    std::error_code candidateError;
    if (std::filesystem::is_regular_file(candidate, candidateError)) { return candidate.string(); }
  }
  const auto systemSleep = std::filesystem::path("/bin/sleep");
  std::error_code sleepError;
  if (std::filesystem::is_regular_file(systemSleep, sleepError)) { return systemSleep.string(); }
  return {};
}

/** @return a ProcessInfoDto request body for POST /process/start */
std::string pcStartBody(const std::string &path, const std::vector<std::string> &args)
{
  std::string body = R"({"id":"","path":")" + path + R"(","args":[)";
  for (std::size_t i = 0; i < args.size(); i++) {
    if (i > 0) { body += ","; }
    body += R"({"arg":")" + args[i] + R"("})";
  }
  body += R"(],"auto_restart":false,"restarts":0,"max_restarts":0,"process_id":0,)"
          R"("runs":false,"exit_code":0,"exit_code_valid":false,)"
          R"("group_name":"","group_id":""})";
  return body;
}

/**
 * ProcessController takes the concrete ProcessService, which the CHANGELOG
 * listed as the reason the 404 switch was untested. That is not a real blocker:
 * ProcessService is constructible from four plain values (process name,
 * environment configuration, configuration, history service) - no complete
 * bootstrap environment and no database are needed, exactly as
 * tests/services/ProcessServiceTest.cpp shows. The tests below therefore use a
 * real ProcessService and close the known gap without touching the API.
 *
 * The ADD_HANDLER_METHODs live in the private section, so the controller is
 * driven through a real httplib server, as in production.
 *
 * onInitialize() is deliberately not called: the monitor thread is not needed
 * for the endpoints under test and would race with the assertions. The
 * ProcessService destructor stops and terminates every remaining child, so no
 * process outlives a test case.
 */
struct ProcessControllerFixture
{
  // must stay the first member: it has to be set before the EnvironmentConfiguration below reads it
  ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_process_controller_test_cfg" };
  std::string m_selfPath;
  std::shared_ptr<EnvironmentConfiguration> m_envConfig;
  std::shared_ptr<Configuration> m_configuration;
  std::shared_ptr<ProcessName> m_processName;
  std::shared_ptr<NoopHistoryService> m_historyService;
  std::shared_ptr<ProcessService> m_processService;
  std::shared_ptr<MockAuthService> m_authService;
  std::shared_ptr<httplib::Server> m_server = std::make_shared<httplib::Server>();
  std::thread m_serverThread;
  int m_port = -1;
  std::unique_ptr<httplib::Client> m_client;

  ProcessControllerFixture()
  {
    static int counter = 0;
    m_selfPath = (std::filesystem::temp_directory_path()
                  / ("pc-controller-self-" + std::to_string(::getpid()) + "-" + std::to_string(++counter)))
                   .string();

    m_envConfig = std::make_shared<EnvironmentConfiguration>();
    m_configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, m_envConfig);
    m_processName = std::make_shared<ProcessName>(m_selfPath);
    m_historyService = std::make_shared<NoopHistoryService>(m_processName);
    m_processService = std::make_shared<ProcessService>(m_processName, m_envConfig, m_configuration, m_historyService);
    m_authService = std::make_shared<MockAuthService>();
  }

  ~ProcessControllerFixture()
  {
    if (m_server != nullptr) { m_server->stop(); }
    if (m_serverThread.joinable()) { m_serverThread.join(); }
  }

  ProcessControllerFixture(const ProcessControllerFixture &) = delete;
  ProcessControllerFixture &operator=(const ProcessControllerFixture &) = delete;

  ProcessController makeController() const { return ProcessController(m_authService, m_processService); }

  /** Register the routes of the controller, bind an ephemeral port and serve */
  void startServing(Controller &controller)
  {
    controller.registerMethods(m_server);
    m_port = m_server->bind_to_any_port("127.0.0.1");
    if (m_port <= 0) { return; }
    m_serverThread = std::thread([this] { m_server->listen_after_bind(); });
    const auto deadline = std::chrono::steady_clock::now() + pcStartupTimeout;
    while (!m_server->is_running() && std::chrono::steady_clock::now() < deadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!m_server->is_running()) { return; }
    m_client = std::make_unique<httplib::Client>("127.0.0.1", m_port);
    m_client->set_connection_timeout(std::chrono::seconds(10));
    m_client->set_read_timeout(std::chrono::seconds(10));
  }

  bool isReady() const { return m_client != nullptr && m_server->is_running(); }

  /** @return the file name of the process this service considers to be itself */
  std::string selfName() const { return m_processName->getProcessName(); }

  /** @return the process infos produced by the controller, parsed back */
  std::vector<ProcessInfoDto> parseProcessInfos(const std::string &json) const
  {
    rapidjson::Document document;
    document.Parse(json.c_str());
    ProcessInfosDto parsed;
    parsed.deserialize(document);
    return parsed.getProcessInfos();
  }

  /** @return the process groups produced by the controller, parsed back */
  std::vector<ProcessGroupDto> parseProcessGroups(const std::string &json) const
  {
    ProcessGroupsDto parsed;
    parsed.deserialize(json);
    return parsed.getProcessGroups();
  }
};

}// namespace

// --------------------------------------------------------------------------------------
// the 404 switch - the gap the CHANGELOG documents
// --------------------------------------------------------------------------------------

TEST_CASE("ProcessController::stopProcessDelete returns 404 for an unknown process id", "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(pcTokenOf(pcAdminUser, pcTokenAdmin));
  // an unknown id must never reach the service that would stop something
  REQUIRE_FALSE(fixture.m_processService->of(pcUnknownProcessId).has_value());

  auto response =
    fixture.m_client->Delete("/process/stop/" + std::string(pcUnknownProcessId), pcAuthJsonHeaders(pcTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 404);
  REQUIRE(response->body.empty());
}

TEST_CASE("ProcessController::terminateProcessDelete returns 404 for an unknown process id", "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(pcTokenOf(pcAdminUser, pcTokenAdmin));

  auto response =
    fixture.m_client->Delete("/process/terminate/" + std::string(pcUnknownProcessId), pcAuthJsonHeaders(pcTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 404);
  REQUIRE(response->body.empty());
}

TEST_CASE("ProcessController::restartProcessPut returns 404 for an unknown process id", "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(pcTokenOf(pcAdminUser, pcTokenAdmin));

  auto response = fixture.m_client->Put("/process/restart/" + std::string(pcUnknownProcessId),
    pcAuthJsonHeaders(pcTokenAdmin),
    std::string(),
    "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 404);
  REQUIRE(response->body.empty());
}

TEST_CASE("ProcessController::resetProcessPost returns 404 for an unknown process id", "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(pcTokenOf(pcAdminUser, pcTokenAdmin));

  auto response = fixture.m_client->Post("/process/reset/" + std::string(pcUnknownProcessId),
    pcAuthJsonHeaders(pcTokenAdmin),
    std::string(),
    "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 404);
  REQUIRE(response->body.empty());
}

TEST_CASE("ProcessController::healthProcessGet returns 404 for an unknown process id", "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(pcTokenOf(pcAdminUser, pcTokenAdmin));

  auto response =
    fixture.m_client->Get("/process/health/" + std::string(pcUnknownProcessId), pcAuthHeaders(pcTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 404);
  REQUIRE(response->body.empty());
}

TEST_CASE(
  "ProcessController::healthProcessGet returns 404 for the own process, which is not a "
  "managed child",
  "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  // ProcessService::of() only knows the processes it spawned; the process itself is reported by
  // allActiveOf()/currentOf() but is never a lifecycle target, so it must answer 404 as well
  const std::string ownId = std::to_string(fixture.m_processService->currentOf().getProcessId());
  REQUIRE_FALSE(fixture.m_processService->of(ownId).has_value());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(pcTokenOf(pcAdminUser, pcTokenAdmin));

  auto response = fixture.m_client->Get("/process/health/" + ownId, pcAuthHeaders(pcTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 404);
  REQUIRE(response->body.empty());
}

// --------------------------------------------------------------------------------------
// authorization - the 404 must not be reachable before the group check
// --------------------------------------------------------------------------------------

TEST_CASE("ProcessController returns 401 for an unknown process id without a bearer token", "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  FORBID_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)));

  auto stop = fixture.m_client->Delete("/process/stop/" + std::string(pcUnknownProcessId));
  auto terminate = fixture.m_client->Delete("/process/terminate/" + std::string(pcUnknownProcessId));
  auto restart = fixture.m_client->Put(
    "/process/restart/" + std::string(pcUnknownProcessId), httplib::Headers{}, std::string(), "application/json");
  auto reset = fixture.m_client->Post(
    "/process/reset/" + std::string(pcUnknownProcessId), httplib::Headers{}, std::string(), "application/json");
  auto health = fixture.m_client->Get("/process/health/" + std::string(pcUnknownProcessId));

  REQUIRE(stop != nullptr);
  REQUIRE(stop->status == 401);
  REQUIRE(terminate != nullptr);
  REQUIRE(terminate->status == 401);
  REQUIRE(restart != nullptr);
  REQUIRE(restart->status == 401);
  REQUIRE(reset != nullptr);
  REQUIRE(reset->status == 401);
  REQUIRE(health != nullptr);
  REQUIRE(health->status == 401);
}

TEST_CASE("ProcessController returns 401 for an unknown process id when the caller is no admin", "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(5)
    .LR_RETURN(pcTokenOf(pcPlainUser, pcTokenPlain));

  auto stop =
    fixture.m_client->Delete("/process/stop/" + std::string(pcUnknownProcessId), pcAuthJsonHeaders(pcTokenPlain));
  auto terminate =
    fixture.m_client->Delete("/process/terminate/" + std::string(pcUnknownProcessId), pcAuthJsonHeaders(pcTokenPlain));
  auto restart = fixture.m_client->Put("/process/restart/" + std::string(pcUnknownProcessId),
    pcAuthJsonHeaders(pcTokenPlain),
    std::string(),
    "application/json");
  auto reset = fixture.m_client->Post("/process/reset/" + std::string(pcUnknownProcessId),
    pcAuthJsonHeaders(pcTokenPlain),
    std::string(),
    "application/json");
  auto health =
    fixture.m_client->Get("/process/health/" + std::string(pcUnknownProcessId), pcAuthHeaders(pcTokenPlain));

  REQUIRE(stop != nullptr);
  REQUIRE(stop->status == 401);
  REQUIRE(terminate != nullptr);
  REQUIRE(terminate->status == 401);
  REQUIRE(restart != nullptr);
  REQUIRE(restart->status == 401);
  REQUIRE(reset != nullptr);
  REQUIRE(reset->status == 401);
  REQUIRE(health != nullptr);
  REQUIRE(health->status == 401);
}

TEST_CASE("ProcessController returns 401 for a user outside Admin-Process and User-Process", "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(pcTokenOf(pcOutsiderUser, pcTokenOutsider));

  auto response =
    fixture.m_client->Get("/process/health/" + std::string(pcUnknownProcessId), pcAuthHeaders(pcTokenOutsider));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE(response->body.empty());
}

TEST_CASE("ProcessController::stopProcessDelete returns 403 for a non json content type", "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(pcTokenOf(pcAdminUser, pcTokenAdmin));

  auto headers = pcAuthHeaders(pcTokenAdmin);
  headers.emplace("Content-Type", "text/plain");
  auto response = fixture.m_client->Delete("/process/stop/" + std::string(pcUnknownProcessId), headers);
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
  REQUIRE(response->body.empty());
}

// --------------------------------------------------------------------------------------
// read endpoints
// --------------------------------------------------------------------------------------

TEST_CASE("ProcessController::allProcessOfGet lists the own process for a matching file name", "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(pcTokenOf(pcPlainUser, pcTokenPlain));

  auto response = fixture.m_client->Get("/process/processes/" + fixture.selfName(), pcAuthHeaders(pcTokenPlain));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);

  const auto infos = fixture.parseProcessInfos(response->body);
  REQUIRE(infos.size() == 1);
  REQUIRE(infos[0].getPath() == fixture.m_selfPath);
  REQUIRE(infos[0].getProcessId() == fixture.m_processService->currentOf().getProcessId());
}

TEST_CASE("ProcessController::allProcessOfGet returns an empty array when the pattern matches nothing",
  "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(pcTokenOf(pcPlainUser, pcTokenPlain));

  auto response = fixture.m_client->Get("/process/processes/pc-no-such-process", pcAuthHeaders(pcTokenPlain));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  REQUIRE(response->body == "[]");
  REQUIRE(fixture.parseProcessInfos(response->body).empty());
}

TEST_CASE("ProcessController::allProcessOfGet returns 400 for an invalid group name pattern", "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(pcTokenOf(pcPlainUser, pcTokenPlain));

  // two wildcard runs: RegexUtils::validatePattern() rejects it as a ReDoS hazard
  auto response = fixture.m_client->Get("/process/processes/a.*b.*c", pcAuthHeaders(pcTokenPlain));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 400);
  REQUIRE(response->body.empty());
}

TEST_CASE("ProcessController::allProcessGroupsOfGet returns an empty array without configured groups",
  "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(pcTokenOf(pcPlainUser, pcTokenPlain));

  auto response = fixture.m_client->Get("/process/groups/pcAnyGroup", pcAuthHeaders(pcTokenPlain));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  REQUIRE(response->body == "[]");
  REQUIRE(fixture.parseProcessGroups(response->body).empty());
}

TEST_CASE("ProcessController::allProcessGroupsOfGet returns 401 without a bearer token", "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  FORBID_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)));

  auto processes = fixture.m_client->Get("/process/processes/.*");
  auto groups = fixture.m_client->Get("/process/groups/.*");
  REQUIRE(processes != nullptr);
  REQUIRE(processes->status == 401);
  REQUIRE(groups != nullptr);
  REQUIRE(groups->status == 401);
}

// --------------------------------------------------------------------------------------
// lifecycle - start, health and stop a real child process
// --------------------------------------------------------------------------------------

TEST_CASE("ProcessController starts, reports and stops a managed process", "[process_controller]")
{
  const std::string sleeper = pcFindSleeperBinary();
  // Not a SKIP: the helper binary is built together with the tests, so a missing one means the
  // build tree is broken.
  REQUIRE(!sleeper.empty());

  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(5)
    .LR_RETURN(pcTokenOf(pcAdminUser, pcTokenAdmin));

  auto started = fixture.m_client->Post(
    "/process/start", pcAuthJsonHeaders(pcTokenAdmin), pcStartBody(sleeper, { "600" }), "application/json");
  REQUIRE(started != nullptr);
  REQUIRE(started->status == 200);
  // the controller assigns a fresh id and does not echo it, so the child is located by its file name
  REQUIRE(fixture.m_processService->allActiveOf().size() == 2);

  const std::string sleeperName = std::filesystem::path(sleeper).filename().string();
  auto listed = fixture.m_client->Get("/process/processes/" + sleeperName, pcAuthHeaders(pcTokenAdmin));
  REQUIRE(listed != nullptr);
  REQUIRE(listed->status == 200);
  const auto infos = fixture.parseProcessInfos(listed->body);
  REQUIRE(infos.size() == 1);
  REQUIRE(infos[0].getPath() == sleeper);
  const std::string childId = infos[0].getId();
  REQUIRE(!childId.empty());

  auto health = fixture.m_client->Get("/process/health/" + childId, pcAuthHeaders(pcTokenAdmin));
  REQUIRE(health != nullptr);
  REQUIRE(health->status == 200);
  rapidjson::Document document;
  document.Parse(health->body.c_str());
  ProcessInfoDto healthDto;
  REQUIRE(healthDto.deserialize(document));
  REQUIRE(healthDto.getId() == childId);
  REQUIRE(healthDto.isRunning());

  auto stopped = fixture.m_client->Delete("/process/stop/" + childId, pcAuthJsonHeaders(pcTokenAdmin));
  REQUIRE(stopped != nullptr);
  REQUIRE(stopped->status == 200);
  REQUIRE(stopped->body.empty());

  // a process that really stopped is erased from the service, so its id is unknown again and the
  // same route answers 404 - the 404 contract verified on a real process, not only on a fake id
  REQUIRE_FALSE(fixture.m_processService->of(childId).has_value());
  auto afterStop = fixture.m_client->Get("/process/health/" + childId, pcAuthHeaders(pcTokenAdmin));
  REQUIRE(afterStop != nullptr);
  REQUIRE(afterStop->status == 404);
  REQUIRE(afterStop->body.empty());
}

TEST_CASE("ProcessController::terminateProcessDelete stops a managed process", "[process_controller]")
{
  const std::string sleeper = pcFindSleeperBinary();
  REQUIRE(!sleeper.empty());

  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(3)
    .LR_RETURN(pcTokenOf(pcAdminUser, pcTokenAdmin));

  auto started = fixture.m_client->Post(
    "/process/start", pcAuthJsonHeaders(pcTokenAdmin), pcStartBody(sleeper, { "600" }), "application/json");
  REQUIRE(started != nullptr);
  REQUIRE(started->status == 200);

  const std::string sleeperName = std::filesystem::path(sleeper).filename().string();
  auto listed = fixture.m_client->Get("/process/processes/" + sleeperName, pcAuthHeaders(pcTokenAdmin));
  REQUIRE(listed != nullptr);
  const auto infos = fixture.parseProcessInfos(listed->body);
  REQUIRE(infos.size() == 1);
  const std::string childId = infos[0].getId();

  auto terminated = fixture.m_client->Delete("/process/terminate/" + childId, pcAuthJsonHeaders(pcTokenAdmin));
  REQUIRE(terminated != nullptr);
  REQUIRE(terminated->status == 200);
  REQUIRE(fixture.m_processService->of(childId).has_value() == false);
}

TEST_CASE("ProcessController::resetProcessPost resets a managed process", "[process_controller]")
{
  const std::string sleeper = pcFindSleeperBinary();
  REQUIRE(!sleeper.empty());

  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(3)
    .LR_RETURN(pcTokenOf(pcAdminUser, pcTokenAdmin));

  auto started = fixture.m_client->Post(
    "/process/start", pcAuthJsonHeaders(pcTokenAdmin), pcStartBody(sleeper, { "600" }), "application/json");
  REQUIRE(started != nullptr);
  REQUIRE(started->status == 200);

  const std::string sleeperName = std::filesystem::path(sleeper).filename().string();
  auto listed = fixture.m_client->Get("/process/processes/" + sleeperName, pcAuthHeaders(pcTokenAdmin));
  REQUIRE(listed != nullptr);
  const auto infos = fixture.parseProcessInfos(listed->body);
  REQUIRE(infos.size() == 1);
  const std::string childId = infos[0].getId();

  auto reset = fixture.m_client->Post(
    "/process/reset/" + childId, pcAuthJsonHeaders(pcTokenAdmin), std::string(), "application/json");
  REQUIRE(reset != nullptr);
  REQUIRE(reset->status == 200);
  // resetOf() clears the failure state but keeps the child tracked
  const auto stillKnown = fixture.m_processService->of(childId);
  REQUIRE(stillKnown.has_value());
  REQUIRE(stillKnown.value()->currentRestarts() == 0);

  fixture.m_processService->terminateOf(*stillKnown.value());
}

TEST_CASE("ProcessController::startProcessPost returns 401 without a bearer token", "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  FORBID_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)));

  auto response =
    fixture.m_client->Post("/process/start", httplib::Headers{}, pcStartBody("/bin/true", {}), "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  // nothing was started
  REQUIRE(fixture.m_processService->allActiveOf().size() == 1);
}

TEST_CASE("ProcessController::startProcessPost returns 401 for a user without Admin-Process", "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(pcTokenOf(pcPlainUser, pcTokenPlain));

  auto response = fixture.m_client->Post(
    "/process/start", pcAuthJsonHeaders(pcTokenPlain), pcStartBody("/bin/true", {}), "application/json");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE(fixture.m_processService->allActiveOf().size() == 1);
}

TEST_CASE("ProcessController::startProcessPost returns 403 for a non json content type", "[process_controller]")
{
  ProcessControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(pcTokenOf(pcAdminUser, pcTokenAdmin));

  auto headers = pcAuthHeaders(pcTokenAdmin);
  headers.emplace("Content-Type", "text/plain");
  auto response = fixture.m_client->Post("/process/start", headers, pcStartBody("/bin/true", {}), "text/plain");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
  REQUIRE(fixture.m_processService->allActiveOf().size() == 1);
}
