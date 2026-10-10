#include <httplib.h>

#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include <lintel/core/persistence/Connection.h>
#include <lintel/core/persistence/ConnectionType.h>
#include <lintel/core/persistence/DatabaseConnectionConfigurations.h>
#include <lintel/core/persistence/Statement.h>
#include <lintel/core/utils/TypeName.h>
#include <lintel/features/base/configuration/Configuration.h>
#include <lintel/features/base/configuration/DatabaseConnectionComponent.h>
#include <lintel/features/base/configuration/DatabaseConnectionEntry.h>
#include <lintel/features/base/configuration/EnvironmentConfiguration.h>
#include <lintel/features/base/controller/HistoryDtos.h>
#include <lintel/features/base/models/Group.h>
#include <lintel/features/base/models/HistoryEntry.h>
#include <lintel/features/base/models/User.h>
#include <lintel/features/base/repositories/GroupRepository.h>
#include <lintel/features/base/repositories/HistoryRepository.h>
#include <lintel/features/http/controllers/HistoryController.h>
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

// `hc` prefix: unity builds merge several test files into one translation unit
// and share one anonymous namespace, so a generic `adminGroup` or `fixture`
// would collide (and shadow local variables, Clang -Wshadow).
constexpr std::chrono::milliseconds hcStartupTimeout{ 5000 };
constexpr char hcTokenAdmin[] = "history-token-admin";
constexpr char hcTokenOutsider[] = "history-token-outsider";

Group hcAdminGroup{ "Admin-History", {}, true };
Group hcUserGroup{ "User-History", {}, true };

User hcAdmin("Hist", "Ory", User::Male, "history@test.com", "historyAdmin", "", { hcAdminGroup });
Group hcUnrelatedGroup{ "Some-Other-Group", {}, true };
User hcOutsider("Out", "Sider", User::Male, "out@test.com", "outsider", "", { hcUnrelatedGroup });

UserToken hcTokenOf(const User &user, const std::string &id)
{
  return UserToken{
    "127.0.0.1", id, std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()), user
  };
}

date::sys_time<std::chrono::microseconds> hcTimestampOf(int seconds)
{
  return date::sys_time<std::chrono::microseconds>{ std::chrono::microseconds{ std::chrono::seconds{ seconds } } };
}

/** @return the Authorization header that carries the given bearer token */
httplib::Headers hcAuthHeaders(const char *token)
{
  return httplib::Headers{ { "Authorization", std::string("Bearer ") + token } };
}

/**
 * The ADD_HANDLER_METHODs of HistoryController live in the private section, so
 * the only reachable entry point is Controller::registerMethods(). The
 * controller is therefore driven through a real httplib server, exactly as in
 * production. That also covers the bearer-token parsing and the "GET without
 * Content-Type defaults to application/json" rule of the ADD_HANDLER_METHOD
 * wrapper, both of which belong to the documented contract in swagger.yaml.
 *
 * HistoryController takes the concrete HistoryRepository, so the success paths
 * need a real SQLite database behind it.
 */
struct HistoryControllerFixture
{
  // must stay the first member: it has to be set before the EnvironmentConfiguration below reads it
  ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_history_controller_test_cfg" };
  std::string m_dbPath;
  std::shared_ptr<EnvironmentConfiguration> m_envConfig;
  std::shared_ptr<Configuration> m_configuration;
  std::shared_ptr<DatabaseConnectionConfigurations> m_connectionConfigurations;
  std::shared_ptr<GroupRepository> m_groupRepository;
  std::shared_ptr<HistoryRepository> m_historyRepository;
  std::shared_ptr<MockAuthService> m_authService;
  std::shared_ptr<httplib::Server> m_server = std::make_shared<httplib::Server>();
  std::thread m_serverThread;
  int m_port = -1;
  std::unique_ptr<httplib::Client> m_client;

  HistoryControllerFixture()
  {
    static int counter = 0;
    m_dbPath = (std::filesystem::temp_directory_path()
                / ("history_controller_test_" + std::to_string(::getpid()) + "_" + std::to_string(++counter) + ".db"))
                 .string();
    std::remove(m_dbPath.c_str());

    db::Connection connection(db::ConnectionType::SQLite, m_dbPath);
    db::Statement statement(connection);
    // The production schema derives inserted_timestamp from CURRENT_TIMESTAMP, so its primary key
    // collapses two entries of the same process/service inserted within one second. The label is
    // part of the key here instead, because the fixture inserts several entries per test case at
    // once; HistoryController only reads, never writes, this table.
    statement.execute(R"(
            CREATE TABLE history (
                process_name       TEXT NOT NULL,
                service_name       TEXT NOT NULL,
                label              TEXT NOT NULL,
                text               TEXT NOT NULL,
                uuid               TEXT NOT NULL,
                created_timestamp  TEXT NOT NULL,
                inserted_timestamp TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                CONSTRAINT history_PK PRIMARY KEY (process_name, service_name, label, inserted_timestamp)
            );
        )");


    m_envConfig = std::make_shared<EnvironmentConfiguration>();
    m_configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, m_envConfig);
    auto entry = std::make_shared<DatabaseConnectionEntry>(
      type_name<DatabaseConnectionComponent>(), m_dbPath, "", "", db::ConnectionType::SQLite, "default", 0, "", true);
    m_configuration->setEntries(std::vector<std::shared_ptr<Entry>>{ entry });
    m_connectionConfigurations = std::make_shared<DatabaseConnectionConfigurations>(m_configuration);
    m_groupRepository = std::make_shared<GroupRepository>(m_connectionConfigurations);
    m_historyRepository = std::make_shared<HistoryRepository>(m_connectionConfigurations);
    m_authService = std::make_shared<MockAuthService>();
  }

  ~HistoryControllerFixture()
  {
    if (m_server != nullptr) { m_server->stop(); }
    if (m_serverThread.joinable()) { m_serverThread.join(); }
    std::remove(m_dbPath.c_str());
  }

  HistoryControllerFixture(const HistoryControllerFixture &) = delete;
  HistoryControllerFixture &operator=(const HistoryControllerFixture &) = delete;

  /** @return a controller wired to the real SQLite history repository */
  HistoryController makeController() const
  {
    return HistoryController(m_authService, m_groupRepository, m_historyRepository);
  }

  /** Register the routes of the controller, bind an ephemeral port and serve */
  void startServing(Controller &controller)
  {
    controller.registerMethods(m_server);
    m_port = m_server->bind_to_any_port("127.0.0.1");
    if (m_port <= 0) { return; }
    m_serverThread = std::thread([this] { m_server->listen_after_bind(); });
    const auto deadline = std::chrono::steady_clock::now() + hcStartupTimeout;
    while (!m_server->is_running() && std::chrono::steady_clock::now() < deadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!m_server->is_running()) { return; }
    m_client = std::make_unique<httplib::Client>("127.0.0.1", m_port);
    m_client->set_connection_timeout(std::chrono::seconds(5));
    m_client->set_read_timeout(std::chrono::seconds(5));
  }

  bool isReady() const { return m_client != nullptr && m_server->is_running(); }

  void insert(const std::string &process,
    const std::string &service,
    const std::string &label,
    const std::string &text,
    int seconds) const
  {
    m_historyRepository->insertOf({ HistoryEntry(process, service, label, text, hcTimestampOf(seconds)) });
  }

  /** @return the history entries serialized by the controller, parsed back */
  std::vector<HistoryDto> parseHistory(const std::string &json) const
  {
    HistoryDtos parsed;
    parsed.deserialize(json);
    return parsed.getHistories();
  }
};

}// namespace

TEST_CASE("HistoryController::historyOfGet returns 401 without a bearer token", "[history_controller]")
{
  HistoryControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  // without a bearer token the ADD_HANDLER_METHOD wrapper never asks the auth service at all
  FORBID_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)));
  FORBID_CALL(*fixture.m_authService, allTokensOf(ANY(const std::string &)));

  auto response = fixture.m_client->Get("/history/worker/main/start");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE(response->body.empty());
}

TEST_CASE(
  "HistoryController::historyOfGet returns 401 for a user outside Admin-History and "
  "User-History",
  "[history_controller]")
{
  HistoryControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(hcTokenOf(hcOutsider, hcTokenOutsider));
  FORBID_CALL(*fixture.m_authService, allTokensOf(ANY(const std::string &)));

  auto response = fixture.m_client->Get("/history/worker/main/start", hcAuthHeaders(hcTokenOutsider));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE(response->body.empty());
}

TEST_CASE("HistoryController::historyOfGet returns 403 for a non json content type", "[history_controller]")
{
  HistoryControllerFixture fixture;
  fixture.insert("worker", "main", "start", "started", 1000);
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(hcTokenOf(hcAdmin, hcTokenAdmin));

  auto headers = hcAuthHeaders(hcTokenAdmin);
  headers.emplace("Content-Type", "text/plain");
  auto response = fixture.m_client->Get("/history/worker/main/start", headers);
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
  REQUIRE(response->body.empty());
}

TEST_CASE("HistoryController::historyOfGet serializes the matching history entries as json array",
  "[history_controller]")
{
  HistoryControllerFixture fixture;
  fixture.insert("worker", "main", "start", "started", 1000);
  fixture.insert("worker", "main", "stop", "stopped", 2000);
  fixture.insert("other", "main", "start", "foreign", 3000);
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(hcTokenOf(hcAdmin, hcTokenAdmin));

  auto response = fixture.m_client->Get("/history/worker/main/start", hcAuthHeaders(hcTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);

  const auto entries = fixture.parseHistory(response->body);
  REQUIRE(entries.size() == 1);
  REQUIRE(entries[0].getProcessName() == "worker");
  REQUIRE(entries[0].getServiceName() == "main");
  REQUIRE(entries[0].getLabel() == "start");
  REQUIRE(entries[0].getText() == "started");
}

TEST_CASE("HistoryController::historyOfGet treats the path segments as regular expressions", "[history_controller]")
{
  HistoryControllerFixture fixture;
  fixture.insert("worker", "main", "start", "started", 1000);
  fixture.insert("worker", "main", "stop", "stopped", 2000);
  fixture.insert("other", "main", "start", "foreign", 3000);
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(hcTokenOf(hcAdmin, hcTokenAdmin));

  auto response = fixture.m_client->Get("/history/work.*/main/.*", hcAuthHeaders(hcTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);

  const auto entries = fixture.parseHistory(response->body);
  REQUIRE(entries.size() == 2);
  REQUIRE(entries[0].getText() == "started");
  REQUIRE(entries[1].getText() == "stopped");
}

TEST_CASE("HistoryController::historyOfGet returns an empty json array when nothing matches", "[history_controller]")
{
  HistoryControllerFixture fixture;
  fixture.insert("worker", "main", "start", "started", 1000);
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(hcTokenOf(hcAdmin, hcTokenAdmin));

  auto response = fixture.m_client->Get("/history/nobody/main/start", hcAuthHeaders(hcTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  REQUIRE(response->body == "[]");
  REQUIRE(fixture.parseHistory(response->body).empty());
}

TEST_CASE("HistoryController::historyOfGet returns 401 for an expired bearer token", "[history_controller]")
{
  HistoryControllerFixture fixture;
  fixture.insert("worker", "main", "start", "started", 1000);
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  // the token is presented, but the auth service no longer knows it
  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(std::optional<UserToken>{});

  auto response = fixture.m_client->Get("/history/worker/main/start", hcAuthHeaders(hcTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE(response->body.empty());
}

TEST_CASE("HistoryController::historyOfGet accepts a user of the User-History group", "[history_controller]")
{
  HistoryControllerFixture fixture;
  fixture.insert("worker", "main", "start", "started", 1000);
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  User plainUser("Plain", "User", User::Male, "plain@test.com", "plain", "", { hcUserGroup });
  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(hcTokenOf(plainUser, hcTokenAdmin));

  auto response = fixture.m_client->Get("/history/worker/main/start", hcAuthHeaders(hcTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  REQUIRE(fixture.parseHistory(response->body).size() == 1);
}
