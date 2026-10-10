#include <httplib.h>

#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include <lintel/features/base/configuration/Configuration.h>
#include <lintel/features/base/configuration/EnvironmentConfiguration.h>
#include <lintel/features/base/configuration/MessageQueueEntry.h>
#include <lintel/features/base/controller/MessageQueueDtos.h>
#include <lintel/features/base/models/Group.h>
#include <lintel/features/base/models/Page.h>
#include <lintel/features/base/models/ProcessName.h>
#include <lintel/features/base/models/User.h>
#include <lintel/features/base/services/IAuthService.h>
#include <lintel/features/base/services/MessageQueueService.h>
#include <lintel/features/http/controllers/MessageQueueController.h>
#include <lintel/features/http/service/ContentType.h>
#include <lintel/features/http/service/HttpStatusCodes.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "../helpers/ScopedEnvironmentVariable.h"
#include "../mocks/MockAuthService.h"
#include "../mocks/MockMessageQueueRepository.h"

using namespace trompeloeil;

namespace {

// `mqc` prefix: unity builds merge several test files into one translation unit
// and share one anonymous namespace, so a generic `adminGroup` or `fixture`
// would collide (and shadow local variables, Clang -Wshadow).
constexpr std::chrono::milliseconds mqcStartupTimeout{ 5000 };
constexpr char mqcTokenAdmin[] = "messagequeue-token-admin";
constexpr char mqcTokenOutsider[] = "messagequeue-token-outsider";

Group mqcAdminGroup{ "Admin-MessageQueue", {}, true };
Group mqcUserGroup{ "User-MessageQueue", {}, true };

User mqcAdmin("Queue", "Admin", User::Male, "queue-admin@test.com", "queueAdmin", "", { mqcAdminGroup });
Group mqcUnrelatedGroup{ "Some-Other-Group", {}, true };
User mqcOutsider("Queue", "Outsider", User::Male, "queue-out@test.com", "queueOut", "", { mqcUnrelatedGroup });

UserToken mqcTokenOf(const User &user, const std::string &id)
{
  return UserToken{
    "127.0.0.1", id, std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()), user
  };
}

/** @return a message queue configuration entry */
MessageQueueEntry mqcEntry(const std::string &process, const std::string &queue, int32_t maxMessages)
{
  return MessageQueueEntry("MessageQueues", process, queue, maxMessages);
}

/** @return the Authorization header that carries the given bearer token */
httplib::Headers mqcAuthHeaders(const char *token)
{
  return httplib::Headers{ { "Authorization", std::string("Bearer ") + token } };
}

/**
 * The ADD_HANDLER_METHODs of MessageQueueController live in the private section,
 * so the only reachable entry point is Controller::registerMethods(). The
 * controller is therefore driven through a real httplib server, exactly as in
 * production.
 *
 * MessageQueueService is a concrete class, but it is cheap to build (no
 * PropertyRegistration, no database) and numberMessagesOf() of an unknown queue
 * reports 0 without creating a named queue, so the real service is used here.
 * The queue names are unique per test so that a stray shared memory queue of
 * another test case cannot change the message count.
 */
struct MessageQueueControllerFixture
{
  // must stay the first member: it has to be set before the EnvironmentConfiguration below reads it
  ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_mqc_test_cfg" };
  std::shared_ptr<EnvironmentConfiguration> m_envConfig;
  std::shared_ptr<Configuration> m_configuration;
  std::shared_ptr<ProcessName> m_processName;
  std::shared_ptr<MockMessageQueueRepository> m_repository;
  std::shared_ptr<MessageQueueService> m_messageQueueService;
  std::shared_ptr<MockAuthService> m_authService;
  std::shared_ptr<httplib::Server> m_server = std::make_shared<httplib::Server>();
  std::thread m_serverThread;
  int m_port = -1;
  std::unique_ptr<httplib::Client> m_client;

  MessageQueueControllerFixture()
  {
    m_envConfig = std::make_shared<EnvironmentConfiguration>();
    m_configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, m_envConfig);
    m_processName = std::make_shared<ProcessName>("messageQueueControllerTest");
    m_repository = std::make_shared<MockMessageQueueRepository>();
    m_messageQueueService = std::make_shared<MessageQueueService>(m_configuration, m_processName, m_repository);
    m_authService = std::make_shared<MockAuthService>();
  }

  ~MessageQueueControllerFixture()
  {
    if (m_server != nullptr) { m_server->stop(); }
    if (m_serverThread.joinable()) { m_serverThread.join(); }
  }

  MessageQueueControllerFixture(const MessageQueueControllerFixture &) = delete;
  MessageQueueControllerFixture &operator=(const MessageQueueControllerFixture &) = delete;

  MessageQueueController makeController() const
  {
    return MessageQueueController(m_authService, m_repository, m_messageQueueService);
  }

  /** Register the routes of the controller, bind an ephemeral port and serve */
  void startServing(Controller &controller)
  {
    controller.registerMethods(m_server);
    m_port = m_server->bind_to_any_port("127.0.0.1");
    if (m_port <= 0) { return; }
    m_serverThread = std::thread([this] { m_server->listen_after_bind(); });
    const auto deadline = std::chrono::steady_clock::now() + mqcStartupTimeout;
    while (!m_server->is_running() && std::chrono::steady_clock::now() < deadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!m_server->is_running()) { return; }
    m_client = std::make_unique<httplib::Client>("127.0.0.1", m_port);
    m_client->set_connection_timeout(std::chrono::seconds(5));
    m_client->set_read_timeout(std::chrono::seconds(5));
  }

  bool isReady() const { return m_client != nullptr && m_server->is_running(); }

  /** @return the plain (non paged) array produced by the controller, parsed back */
  std::vector<MessageQueueDto> parseArray(const std::string &json) const
  {
    MessageQueueDtos parsed;
    parsed.deserialize(json);
    return parsed.getMessageQueueDtos();
  }

  /** @return the paging envelope produced by the controller, parsed back */
  MessageQueuesPageDto parseEnvelope(const std::string &json) const
  {
    MessageQueuesPageDto parsed;
    parsed.deserialize(json);
    return parsed;
  }
};

}// namespace

TEST_CASE("MessageQueueController::messageQueueOfGet returns 401 without a bearer token", "[messagequeue_controller]")
{
  MessageQueueControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  // without a bearer token the wrapper never asks the auth service, and the
  // request never reaches the repository
  FORBID_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)));
  FORBID_CALL(*fixture.m_repository, allOf(ANY(const std::string &), ANY(const std::string &)));
  FORBID_CALL(*fixture.m_repository,
    pageOf(
      ANY(const std::string &), ANY(const std::string &), ANY(const std::optional<std::string> &), ANY(std::size_t)));

  auto response = fixture.m_client->Get("/messageQueue/worker/.*");
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE(response->body.empty());
}

TEST_CASE(
  "MessageQueueController::messageQueueOfGet returns 401 for a user outside "
  "Admin-MessageQueue and User-MessageQueue",
  "[messagequeue_controller]")
{
  MessageQueueControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(mqcTokenOf(mqcOutsider, mqcTokenOutsider));
  FORBID_CALL(*fixture.m_repository, allOf(ANY(const std::string &), ANY(const std::string &)));

  auto response = fixture.m_client->Get("/messageQueue/worker/.*", mqcAuthHeaders(mqcTokenOutsider));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 401);
  REQUIRE(response->body.empty());
}

TEST_CASE("MessageQueueController::messageQueueOfGet returns 403 for a non json content type",
  "[messagequeue_controller]")
{
  MessageQueueControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(mqcTokenOf(mqcAdmin, mqcTokenAdmin));
  FORBID_CALL(*fixture.m_repository, allOf(ANY(const std::string &), ANY(const std::string &)));

  auto headers = mqcAuthHeaders(mqcTokenAdmin);
  headers.emplace("Content-Type", "text/plain");
  auto response = fixture.m_client->Get("/messageQueue/worker/.*", headers);
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 403);
  REQUIRE(response->body.empty());
}

TEST_CASE("MessageQueueController::messageQueueOfGet serializes the repository entries as json array",
  "[messagequeue_controller]")
{
  MessageQueueControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(mqcTokenOf(mqcAdmin, mqcTokenAdmin));
  REQUIRE_CALL(*fixture.m_repository, allOf("worker", "queue_.*"))
    .TIMES(1)
    .LR_RETURN(
      std::vector<MessageQueueEntry>{ mqcEntry("worker", "mqc-alpha", 10), mqcEntry("worker", "mqc-beta", 20) });

  auto response = fixture.m_client->Get("/messageQueue/worker/queue_.*", mqcAuthHeaders(mqcTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);

  const auto entries = fixture.parseArray(response->body);
  REQUIRE(entries.size() == 2);
  REQUIRE(entries[0].getName() == "mqc-alpha");
  REQUIRE(entries[0].getProcess() == "worker");
  REQUIRE(entries[0].getMaxMessages() == 10);
  // no process-global named queue exists for these entries, so the count is 0
  REQUIRE(entries[0].getMessages() == 0);
  REQUIRE(entries[1].getName() == "mqc-beta");
  REQUIRE(entries[1].getMaxMessages() == 20);
}

TEST_CASE(
  "MessageQueueController::messageQueueOfGet returns an empty array when the repository is "
  "empty",
  "[messagequeue_controller]")
{
  MessageQueueControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(mqcTokenOf(mqcAdmin, mqcTokenAdmin));
  REQUIRE_CALL(*fixture.m_repository, allOf("nobody", "nothing")).TIMES(1).LR_RETURN(std::vector<MessageQueueEntry>{});

  auto response = fixture.m_client->Get("/messageQueue/nobody/nothing", mqcAuthHeaders(mqcTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  REQUIRE(response->body == "[]");
  REQUIRE(fixture.parseArray(response->body).empty());
}

TEST_CASE(
  "MessageQueueController::messageQueueOfGet returns the paging envelope when a limit is "
  "given",
  "[messagequeue_controller]")
{
  MessageQueueControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(mqcTokenOf(mqcAdmin, mqcTokenAdmin));
  // the unpaged allOf() must not be used once paging is active
  FORBID_CALL(*fixture.m_repository, allOf(ANY(const std::string &), ANY(const std::string &)));
  REQUIRE_CALL(*fixture.m_repository,
    pageOf("worker", "mqc-page", std::optional<std::string>{ "mqc-page-0" }, static_cast<std::size_t>(1)))
    .TIMES(1)
    .LR_RETURN(Page<MessageQueueEntry>(
      { mqcEntry("worker", "mqc-page-0", 5) }, true, std::optional<std::string>{ "mqc-page-0" }));

  auto response =
    fixture.m_client->Get("/messageQueue/worker/mqc-page?after=mqc-page-0&limit=1", mqcAuthHeaders(mqcTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);

  const auto envelope = fixture.parseEnvelope(response->body);
  REQUIRE(envelope.getMessageQueueDtos().size() == 1);
  REQUIRE(envelope.getMessageQueueDtos()[0].getName() == "mqc-page-0");
  REQUIRE(envelope.hasMore());
  REQUIRE(envelope.getNextAfter().has_value());
  REQUIRE(envelope.getNextAfter().value() == "mqc-page-0");
}

TEST_CASE("MessageQueueController::messageQueueOfGet returns 400 for a limit of zero", "[messagequeue_controller]")
{
  MessageQueueControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(mqcTokenOf(mqcAdmin, mqcTokenAdmin));
  // pagingParamsOf() rejects the request before any repository access happens
  FORBID_CALL(*fixture.m_repository, allOf(ANY(const std::string &), ANY(const std::string &)));
  FORBID_CALL(*fixture.m_repository,
    pageOf(
      ANY(const std::string &), ANY(const std::string &), ANY(const std::optional<std::string> &), ANY(std::size_t)));

  auto response = fixture.m_client->Get("/messageQueue/worker/.*?limit=0", mqcAuthHeaders(mqcTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 400);
  REQUIRE(response->body == "bad request");
}

TEST_CASE("MessageQueueController::messageQueueOfGet returns 400 for a non numeric limit", "[messagequeue_controller]")
{
  MessageQueueControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(mqcTokenOf(mqcAdmin, mqcTokenAdmin));
  FORBID_CALL(*fixture.m_repository, allOf(ANY(const std::string &), ANY(const std::string &)));

  auto response = fixture.m_client->Get("/messageQueue/worker/.*?limit=abc", mqcAuthHeaders(mqcTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 400);
  REQUIRE(response->body == "bad request");
}

TEST_CASE("MessageQueueController::messageQueueOfGet accepts a user of the User-MessageQueue group",
  "[messagequeue_controller]")
{
  MessageQueueControllerFixture fixture;
  auto controller = fixture.makeController();
  fixture.startServing(controller);
  REQUIRE(fixture.isReady());

  User plainUser("Queue", "Plain", User::Male, "queue-plain@test.com", "queuePlain", "", { mqcUserGroup });
  REQUIRE_CALL(*fixture.m_authService, onAccessOf(ANY(const UserTokenLogin &)))
    .TIMES(1)
    .LR_RETURN(mqcTokenOf(plainUser, mqcTokenAdmin));
  REQUIRE_CALL(*fixture.m_repository, allOf("worker", "mqc-plain"))
    .TIMES(1)
    .LR_RETURN(std::vector<MessageQueueEntry>{ mqcEntry("worker", "mqc-plain", 3) });

  auto response = fixture.m_client->Get("/messageQueue/worker/mqc-plain", mqcAuthHeaders(mqcTokenAdmin));
  REQUIRE(response != nullptr);
  REQUIRE(response->status == 200);
  REQUIRE(fixture.parseArray(response->body).size() == 1);
}
