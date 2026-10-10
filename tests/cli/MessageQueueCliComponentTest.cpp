#include <catch2/catch_all.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <httplib.h>

#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/configuration/Configuration.h"
#include "lintel/features/base/configuration/EnvironmentConfiguration.h"
#include "lintel/features/base/configuration/MessageQueueEntry.h"
#include "lintel/features/base/controller/MessageQueueDtos.h"
#include "lintel/features/base/controller/UserDto.h"
#include "lintel/features/cli/components/MessageQueueCliComponent.h"
#include "lintel/features/http/configuration/ClientConfiguration.h"
#include "lintel/features/http/configuration/HttpComponent.h"
#include "lintel/features/http/configuration/HttpEntry.h"
#include "lintel/features/http/provider/ClientProvider.h"

namespace {

/** RAII redirector for `std::cout`, restored on destruction even when a REQUIRE throws. */
class mqCliCoutCapture
{
public:
  mqCliCoutCapture() : m_oldBuffer(std::cout.rdbuf(m_stream.rdbuf())) {}

  ~mqCliCoutCapture() { std::cout.rdbuf(m_oldBuffer); }

  mqCliCoutCapture(const mqCliCoutCapture &) = delete;
  mqCliCoutCapture &operator=(const mqCliCoutCapture &) = delete;

  [[nodiscard]] std::string str() const { return m_stream.str(); }

private:
  std::ostringstream m_stream;
  std::streambuf *m_oldBuffer;
};

/** RAII redirector for `std::cerr`, used for the error paths of onCommand(). */
class mqCliCerrCapture
{
public:
  mqCliCerrCapture() : m_oldBuffer(std::cerr.rdbuf(m_stream.rdbuf())) {}

  ~mqCliCerrCapture() { std::cerr.rdbuf(m_oldBuffer); }

  mqCliCerrCapture(const mqCliCerrCapture &) = delete;
  mqCliCerrCapture &operator=(const mqCliCerrCapture &) = delete;

  [[nodiscard]] std::string str() const { return m_stream.str(); }

private:
  std::ostringstream m_stream;
  std::streambuf *m_oldBuffer;
};

/**
 * `MesssageQueueApi` has no virtual seam, so the component is driven against a loopback HTTP
 * server that records every request and answers with a canned body.
 */
class mqCliBackend
{
public:
  mqCliBackend()
  {
    m_server.set_pre_routing_handler([this](const httplib::Request &request, httplib::Response &response) {
      paths.push_back(request.path);
      methods.emplace_back(request.method);
      response.set_content(m_responseBody, "application/json");
      return httplib::Server::HandlerResponse::Handled;
    });
    m_port = m_server.bind_to_any_port("127.0.0.1");
    m_thread = std::thread([this]() { m_server.listen_after_bind(); });
    // Server::stop() is a no-op until the accept loop is up. Without this wait a
    // backend that is created and destroyed without a single request leaves the
    // accept loop blocked in accept() and the join in the destructor never returns.
    const auto mqCliDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!m_server.is_running() && std::chrono::steady_clock::now() < mqCliDeadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  ~mqCliBackend()
  {
    m_server.stop();
    if (m_thread.joinable()) { m_thread.join(); }
  }

  mqCliBackend(const mqCliBackend &) = delete;
  mqCliBackend &operator=(const mqCliBackend &) = delete;

  void answerWith(const std::string &body) { m_responseBody = body; }

  [[nodiscard]] int port() const { return m_port; }

  /** @return true once the accept loop is up, so stop() can shut the server down */
  [[nodiscard]] bool running() const { return m_server.is_running(); }

  std::vector<std::string> paths;
  std::vector<std::string> methods;

private:
  httplib::Server m_server;
  std::thread m_thread;
  int m_port{ -1 };
  std::string m_responseBody{ "[]" };
};

/** Client provider whose only client entry points at the loopback backend. */
std::shared_ptr<ClientProvider> mqCliProviderOf(int port)
{
  static const std::string kMqCliAbsentConfigDir = "/nonexistent/lintel-mq-cli-test-config";
  auto environment = std::make_shared<EnvironmentConfiguration>();
  environment->overrides(EnvironmentConfiguration::ConfigDirectory, kMqCliAbsentConfigDir);
  auto clientConfiguration = std::make_shared<ClientConfiguration>("127.0.0.1",
    port,
    std::chrono::milliseconds(2000),
    std::chrono::milliseconds(2000),
    std::chrono::milliseconds(2000));
  std::vector<std::shared_ptr<Entry>> entries{ std::make_shared<HttpEntry>(
    type_name<HttpComponent>(), clientConfiguration) };
  auto configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, environment);
  configuration->setEntries(entries);
  return std::make_shared<ClientProvider>(configuration);
}

/** @return the JSON body MessageQueueController would send for one queue */
std::string
  mqCliOneQueueBody(const std::string &processName, const std::string &queueName, int32_t maxMessages, int32_t messages)
{
  MessageQueueEntry mqCliEntry("MessageQueueComponent", processName, queueName, maxMessages);
  MessageQueueDtos mqCliDtos(std::vector<std::pair<MessageQueueEntry, int32_t>>{ { mqCliEntry, messages } });
  return mqCliDtos.JsonSerializable::serialize();
}

}// namespace

TEST_CASE("MessageQueueCliComponent: onShowMenu prints the no-submenu notice")
{
  auto mqCliApi = std::make_shared<MesssageQueueApi>(mqCliProviderOf(1));
  MessageQueueCliComponent mqCliComponent(mqCliApi);

  mqCliCoutCapture mqCliOut;
  mqCliComponent.onShowMenu();

  REQUIRE(mqCliOut.str().find("No submenu available!") != std::string::npos);
}

TEST_CASE("MessageQueueCliComponent: onMenu returns false for an unknown component")
{
  auto mqCliApi = std::make_shared<MesssageQueueApi>(mqCliProviderOf(1));
  MessageQueueCliComponent mqCliComponent(mqCliApi);

  // CommandLineService only prints "Invalid command" when the component reports false.
  REQUIRE(mqCliComponent.onMenu("History") == false);
}

TEST_CASE("MessageQueueCliComponent: onMenu returns false for its own alias")
{
  auto mqCliApi = std::make_shared<MesssageQueueApi>(mqCliProviderOf(1));
  MessageQueueCliComponent mqCliComponent(mqCliApi);

  const bool mqCliAccepted = mqCliComponent.onMenu(std::string(mqCliComponent.getAlias()));
  REQUIRE(mqCliAccepted == false);
}

TEST_CASE("MessageQueueCliComponent: onHelp prints the header and every command with its arguments")
{
  auto mqCliApi = std::make_shared<MesssageQueueApi>(mqCliProviderOf(1));
  MessageQueueCliComponent mqCliComponent(mqCliApi);

  mqCliCoutCapture mqCliOut;
  mqCliComponent.onHelp();
  const std::string mqCliHelp = mqCliOut.str();

  REQUIRE(mqCliHelp.find("Help Menu of MessageQueue [Mq]:") != std::string::npos);
  REQUIRE(mqCliHelp.find("show_message_queues") != std::string::npos);
  REQUIRE(mqCliHelp.find("--process-name, -p") != std::string::npos);
  REQUIRE(mqCliHelp.find("--message-queue-name, -mq") != std::string::npos);
}

TEST_CASE("MessageQueueCliComponent: printCommandList lists the registered commands")
{
  auto mqCliApi = std::make_shared<MesssageQueueApi>(mqCliProviderOf(1));
  MessageQueueCliComponent mqCliComponent(mqCliApi);

  mqCliCoutCapture mqCliOut;
  mqCliComponent.printCommandList({});
  const std::string mqCliList = mqCliOut.str();

  REQUIRE(mqCliList.find("show_message_queues") != std::string::npos);
}

TEST_CASE("MessageQueueCliComponent: printCommandList appends the sub-menu aliases")
{
  auto mqCliApi = std::make_shared<MesssageQueueApi>(mqCliProviderOf(1));
  MessageQueueCliComponent mqCliComponent(mqCliApi);

  mqCliCoutCapture mqCliOut;
  mqCliComponent.printCommandList({ "exit", "help" });
  const std::string mqCliList = mqCliOut.str();

  REQUIRE(mqCliList.find("show_message_queues") != std::string::npos);
  REQUIRE(mqCliList.find("exit") != std::string::npos);
  REQUIRE(mqCliList.find("help") != std::string::npos);
}

TEST_CASE("MessageQueueCliComponent: allCommandsOf returns the single registered command")
{
  auto mqCliApi = std::make_shared<MesssageQueueApi>(mqCliProviderOf(1));
  MessageQueueCliComponent mqCliComponent(mqCliApi);

  const std::vector<std::string> mqCliCommands = mqCliComponent.allCommandsOf();
  const bool mqCliHasShowMessageQueues =
    std::find(mqCliCommands.begin(), mqCliCommands.end(), "show_message_queues") != mqCliCommands.end();

  REQUIRE(mqCliCommands.size() == 1);
  REQUIRE(mqCliHasShowMessageQueues);
}

TEST_CASE("MessageQueueCliComponent: the component registers itself under name and alias")
{
  auto mqCliApi = std::make_shared<MesssageQueueApi>(mqCliProviderOf(1));
  MessageQueueCliComponent mqCliComponent(mqCliApi);

  REQUIRE(mqCliComponent.getName() == "MessageQueue");
  REQUIRE(mqCliComponent.getAlias() == "Mq");
}

TEST_CASE("MessageQueueCliComponent: onExit confirms the exit")
{
  auto mqCliApi = std::make_shared<MesssageQueueApi>(mqCliProviderOf(1));
  MessageQueueCliComponent mqCliComponent(mqCliApi);

  REQUIRE(mqCliComponent.onExit() == true);
}

TEST_CASE("MessageQueueCliComponent: show_message_queues without flags asks for every queue")
{
  mqCliBackend mqCliServer;
  REQUIRE(mqCliServer.port() > 0);
  REQUIRE(mqCliServer.running());
  auto mqCliApi = std::make_shared<MesssageQueueApi>(mqCliProviderOf(mqCliServer.port()));
  MessageQueueCliComponent mqCliComponent(mqCliApi);

  mqCliCoutCapture mqCliOut;
  mqCliComponent.onCommand(UserDto{}, "show_message_queues", {});
  const std::string mqCliPrinted = mqCliOut.str();

  REQUIRE(mqCliServer.paths.size() == 1);
  REQUIRE(mqCliServer.paths[0] == "/messageQueue/.*/.*");
  REQUIRE(mqCliServer.methods[0] == "GET");
  // header row only for an empty answer
  REQUIRE(mqCliPrinted.find("MaxMessages") != std::string::npos);
}

TEST_CASE("MessageQueueCliComponent: show_message_queues -p -mq forwards both filters")
{
  mqCliBackend mqCliServer;
  auto mqCliApi = std::make_shared<MesssageQueueApi>(mqCliProviderOf(mqCliServer.port()));
  MessageQueueCliComponent mqCliComponent(mqCliApi);

  mqCliCoutCapture mqCliOut;
  mqCliComponent.onCommand(
    UserDto{}, "show_message_queues", { "--process-name", "myprocess", "--message-queue-name", "myqueue" });

  REQUIRE(mqCliServer.paths.size() == 1);
  REQUIRE(mqCliServer.paths[0] == "/messageQueue/myprocess/myqueue");
}

TEST_CASE("MessageQueueCliComponent: show_message_queues accepts the --flag=value form")
{
  mqCliBackend mqCliServer;
  auto mqCliApi = std::make_shared<MesssageQueueApi>(mqCliProviderOf(mqCliServer.port()));
  MessageQueueCliComponent mqCliComponent(mqCliApi);

  mqCliCoutCapture mqCliOut;
  mqCliComponent.onCommand(UserDto{}, "show_message_queues", { "-mq=onlyqueue" });

  REQUIRE(mqCliServer.paths.size() == 1);
  REQUIRE(mqCliServer.paths[0] == "/messageQueue/.*/onlyqueue");
}

TEST_CASE("MessageQueueCliComponent: show_message_queues keeps the wildcard for the process filter")
{
  mqCliBackend mqCliServer;
  auto mqCliApi = std::make_shared<MesssageQueueApi>(mqCliProviderOf(mqCliServer.port()));
  MessageQueueCliComponent mqCliComponent(mqCliApi);

  mqCliCoutCapture mqCliOut;
  mqCliComponent.onCommand(UserDto{}, "show_message_queues", { "-p", "onlyprocess" });

  REQUIRE(mqCliServer.paths.size() == 1);
  REQUIRE(mqCliServer.paths[0] == "/messageQueue/onlyprocess/.*");
}

TEST_CASE("MessageQueueCliComponent: show_message_queues prints the queues returned by the api")
{
  mqCliBackend mqCliServer;
  mqCliServer.answerWith(mqCliOneQueueBody("returned-process", "returned-queue", 42, 7));
  auto mqCliApi = std::make_shared<MesssageQueueApi>(mqCliProviderOf(mqCliServer.port()));
  MessageQueueCliComponent mqCliComponent(mqCliApi);

  mqCliCoutCapture mqCliOut;
  mqCliComponent.onCommand(UserDto{}, "show_message_queues", {});
  const std::string mqCliPrinted = mqCliOut.str();

  REQUIRE(mqCliServer.paths.size() == 1);
  REQUIRE(mqCliPrinted.find("returned-process") != std::string::npos);
  REQUIRE(mqCliPrinted.find("returned-queue") != std::string::npos);
  REQUIRE(mqCliPrinted.find("42") != std::string::npos);
  REQUIRE(mqCliPrinted.find("7") != std::string::npos);

  // the returned row is numbered 1 in the "No." column
  const std::size_t mqCliCell = mqCliPrinted.find("returned-process");
  REQUIRE(mqCliCell != std::string::npos);
  const std::size_t mqCliLineStart = mqCliPrinted.rfind('\n', mqCliCell);
  REQUIRE(mqCliLineStart != std::string::npos);
  REQUIRE(mqCliPrinted.compare(mqCliLineStart + 1, 3, "| 1") == 0);
}

TEST_CASE("MessageQueueCliComponent: an unknown command never reaches the api")
{
  mqCliBackend mqCliServer;
  auto mqCliApi = std::make_shared<MesssageQueueApi>(mqCliProviderOf(mqCliServer.port()));
  MessageQueueCliComponent mqCliComponent(mqCliApi);

  mqCliCoutCapture mqCliOut;
  mqCliComponent.onCommand(UserDto{}, "show_queues", { "-p", "ignored" });
  const std::string mqCliPrinted = mqCliOut.str();

  REQUIRE(mqCliServer.paths.empty());
  REQUIRE(mqCliPrinted.empty());
}

TEST_CASE("MessageQueueCliComponent: show_message_queues without a value for -p prints an error")
{
  mqCliBackend mqCliServer;
  auto mqCliApi = std::make_shared<MesssageQueueApi>(mqCliProviderOf(mqCliServer.port()));
  MessageQueueCliComponent mqCliComponent(mqCliApi);

  mqCliCerrCapture mqCliErr;
  mqCliComponent.onCommand(UserDto{}, "show_message_queues", { "-p" });
  const std::string mqCliReported = mqCliErr.str();

  REQUIRE(mqCliServer.paths.empty());
  REQUIRE(mqCliReported.find("ERROR") != std::string::npos);
}