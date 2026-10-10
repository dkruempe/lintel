#include <catch2/catch_all.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <httplib.h>

#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/configuration/Configuration.h"
#include "lintel/features/base/configuration/EnvironmentConfiguration.h"
#include "lintel/features/base/controller/HistoryDtos.h"
#include "lintel/features/base/controller/UserDto.h"
#include "lintel/features/base/models/HistoryEntry.h"
#include "lintel/features/cli/components/HistoryCliComponent.h"
#include "lintel/features/http/configuration/ClientConfiguration.h"
#include "lintel/features/http/configuration/HttpComponent.h"
#include "lintel/features/http/configuration/HttpEntry.h"
#include "lintel/features/http/provider/ClientProvider.h"

namespace {

/**
 * RAII redirector for `std::cout`; restores the original buffer on destruction so a failing
 * REQUIRE cannot leave the rest of the test binary writing into a dead stream.
 */
class histCliCoutCapture
{
public:
  histCliCoutCapture() : m_oldBuffer(std::cout.rdbuf(m_stream.rdbuf())) {}

  ~histCliCoutCapture() { std::cout.rdbuf(m_oldBuffer); }

  histCliCoutCapture(const histCliCoutCapture &) = delete;
  histCliCoutCapture &operator=(const histCliCoutCapture &) = delete;

  [[nodiscard]] std::string str() const { return m_stream.str(); }

private:
  std::ostringstream m_stream;
  std::streambuf *m_oldBuffer;
};

/** RAII redirector for `std::cerr`, used for the error paths of onCommand(). */
class histCliCerrCapture
{
public:
  histCliCerrCapture() : m_oldBuffer(std::cerr.rdbuf(m_stream.rdbuf())) {}

  ~histCliCerrCapture() { std::cerr.rdbuf(m_oldBuffer); }

  histCliCerrCapture(const histCliCerrCapture &) = delete;
  histCliCerrCapture &operator=(const histCliCerrCapture &) = delete;

  [[nodiscard]] std::string str() const { return m_stream.str(); }

private:
  std::ostringstream m_stream;
  std::streambuf *m_oldBuffer;
};

/**
 * `HistoryApi` has no virtual seam (unlike `UserApi`), so the only faithful way to observe what
 * the CLI component asks for is to let it talk to a real HTTP client. This loopback server
 * records every request and answers with a canned body.
 */
class histCliBackend
{
public:
  histCliBackend()
  {
    m_server.set_pre_routing_handler([this](const httplib::Request &request, httplib::Response &response) {
      paths.push_back(request.path);
      methods.emplace_back(request.method);
      bodies.push_back(request.body);
      response.set_content(m_responseBody, "application/json");
      return httplib::Server::HandlerResponse::Handled;
    });
    m_port = m_server.bind_to_any_port("127.0.0.1");
    m_thread = std::thread([this]() { m_server.listen_after_bind(); });
    // Server::stop() is a no-op until the accept loop is up. Without this wait a
    // backend that is created and destroyed without a single request leaves the
    // accept loop blocked in accept() and the join in the destructor never returns.
    const auto histCliDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!m_server.is_running() && std::chrono::steady_clock::now() < histCliDeadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  ~histCliBackend()
  {
    m_server.stop();
    if (m_thread.joinable()) { m_thread.join(); }
  }

  histCliBackend(const histCliBackend &) = delete;
  histCliBackend &operator=(const histCliBackend &) = delete;

  void answerWith(const std::string &body) { m_responseBody = body; }

  [[nodiscard]] int port() const { return m_port; }

  /** @return true once the accept loop is up, so stop() can shut the server down */
  [[nodiscard]] bool running() const { return m_server.is_running(); }

  /** @return every path the component requested, in order */
  std::vector<std::string> paths;
  std::vector<std::string> methods;
  std::vector<std::string> bodies;

private:
  httplib::Server m_server;
  std::thread m_thread;
  int m_port{ -1 };
  std::string m_responseBody{ "[]" };
};

/**
 * Build a `ClientProvider` whose only client entry points at the loopback backend. The
 * configuration directory is redirected to a path that does not exist so that constructing the
 * `Configuration` cannot pick up the developer's real bootstrap.xml.
 */
std::shared_ptr<ClientProvider> histCliProviderOf(int port)
{
  static const std::string kHistCliAbsentConfigDir = "/nonexistent/lintel-cli-test-config";
  auto environment = std::make_shared<EnvironmentConfiguration>();
  environment->overrides(EnvironmentConfiguration::ConfigDirectory, kHistCliAbsentConfigDir);
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

/** @return a JSON body containing one history entry, exactly as HistoryController would send it */
std::string histCliOneEntryBody(const std::string &processName,
  const std::string &serviceName,
  const std::string &label,
  const std::string &text)
{
  HistoryEntry histCliEntry(processName,
    serviceName,
    label,
    text,
    std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()));
  HistoryDtos histCliDtos(std::vector<HistoryEntry>{ histCliEntry });
  return histCliDtos.JsonSerializable::serialize();
}

}// namespace

TEST_CASE("HistoryCliComponent: onShowMenu prints the no-submenu notice")
{
  auto histCliApi = std::make_shared<HistoryApi>(histCliProviderOf(1));
  HistoryCliComponent histCliComponent(histCliApi);

  histCliCoutCapture histCliOut;
  histCliComponent.onShowMenu();

  REQUIRE(histCliOut.str().find("No submenu available!") != std::string::npos);
}

TEST_CASE("HistoryCliComponent: onMenu returns false for an unknown component")
{
  auto histCliApi = std::make_shared<HistoryApi>(histCliProviderOf(1));
  HistoryCliComponent histCliComponent(histCliApi);

  // CommandLineService prints "Invalid command" only when the component reports false; a
  // component with sub-menus would report true here.
  REQUIRE(histCliComponent.onMenu("Process") == false);
}

TEST_CASE("HistoryCliComponent: onMenu returns false for its own alias")
{
  auto histCliApi = std::make_shared<HistoryApi>(histCliProviderOf(1));
  HistoryCliComponent histCliComponent(histCliApi);

  const bool histCliAccepted = histCliComponent.onMenu(std::string(histCliComponent.getAlias()));
  REQUIRE(histCliAccepted == false);
}

TEST_CASE("HistoryCliComponent: onHelp prints the header and every command with its arguments")
{
  auto histCliApi = std::make_shared<HistoryApi>(histCliProviderOf(1));
  HistoryCliComponent histCliComponent(histCliApi);

  histCliCoutCapture histCliOut;
  histCliComponent.onHelp();
  const std::string histCliHelp = histCliOut.str();

  REQUIRE(histCliHelp.find("Help Menu of History [Hist]:") != std::string::npos);
  REQUIRE(histCliHelp.find("show_histories") != std::string::npos);
  REQUIRE(histCliHelp.find("--process-name, -p") != std::string::npos);
  REQUIRE(histCliHelp.find("--service-name, -s") != std::string::npos);
  REQUIRE(histCliHelp.find("--label, -l") != std::string::npos);
}

TEST_CASE("HistoryCliComponent: printCommandList lists the registered commands")
{
  auto histCliApi = std::make_shared<HistoryApi>(histCliProviderOf(1));
  HistoryCliComponent histCliComponent(histCliApi);

  histCliCoutCapture histCliOut;
  histCliComponent.printCommandList({});
  const std::string histCliList = histCliOut.str();

  REQUIRE(histCliList.find("show_histories") != std::string::npos);
}

TEST_CASE("HistoryCliComponent: printCommandList appends the sub-menu aliases")
{
  auto histCliApi = std::make_shared<HistoryApi>(histCliProviderOf(1));
  HistoryCliComponent histCliComponent(histCliApi);

  histCliCoutCapture histCliOut;
  histCliComponent.printCommandList({ "exit", "help" });
  const std::string histCliList = histCliOut.str();

  REQUIRE(histCliList.find("show_histories") != std::string::npos);
  REQUIRE(histCliList.find("exit") != std::string::npos);
  REQUIRE(histCliList.find("help") != std::string::npos);
}

TEST_CASE("HistoryCliComponent: allCommandsOf returns the single registered command")
{
  auto histCliApi = std::make_shared<HistoryApi>(histCliProviderOf(1));
  HistoryCliComponent histCliComponent(histCliApi);

  const std::vector<std::string> histCliCommands = histCliComponent.allCommandsOf();
  const bool histCliHasShowHistories =
    std::find(histCliCommands.begin(), histCliCommands.end(), "show_histories") != histCliCommands.end();

  REQUIRE(histCliCommands.size() == 1);
  REQUIRE(histCliHasShowHistories);
}

TEST_CASE("HistoryCliComponent: the component registers itself under name and alias")
{
  auto histCliApi = std::make_shared<HistoryApi>(histCliProviderOf(1));
  HistoryCliComponent histCliComponent(histCliApi);

  REQUIRE(histCliComponent.getName() == "History");
  REQUIRE(histCliComponent.getAlias() == "Hist");
}

TEST_CASE("HistoryCliComponent: onExit confirms the exit")
{
  auto histCliApi = std::make_shared<HistoryApi>(histCliProviderOf(1));
  HistoryCliComponent histCliComponent(histCliApi);

  REQUIRE(histCliComponent.onExit() == true);
}

TEST_CASE("HistoryCliComponent: show_histories without flags asks the api for every entry")
{
  histCliBackend histCliServer;
  REQUIRE(histCliServer.port() > 0);
  REQUIRE(histCliServer.running());
  auto histCliApi = std::make_shared<HistoryApi>(histCliProviderOf(histCliServer.port()));
  HistoryCliComponent histCliComponent(histCliApi);

  histCliCoutCapture histCliOut;
  histCliComponent.onCommand(UserDto{}, "show_histories", {});
  const std::string histCliPrinted = histCliOut.str();

  REQUIRE(histCliServer.paths.size() == 1);
  REQUIRE(histCliServer.paths[0] == "/history/.*/.*/.*");
  REQUIRE(histCliServer.methods[0] == "GET");
  // header row only: an empty answer still produces the documented table
  REQUIRE(histCliPrinted.find("No.") != std::string::npos);
  REQUIRE(histCliPrinted.find("Created Timestamp") != std::string::npos);
}

TEST_CASE("HistoryCliComponent: show_histories -p -s -l forwards all three filters")
{
  histCliBackend histCliServer;
  auto histCliApi = std::make_shared<HistoryApi>(histCliProviderOf(histCliServer.port()));
  HistoryCliComponent histCliComponent(histCliApi);

  histCliCoutCapture histCliOut;
  histCliComponent.onCommand(UserDto{},
    "show_histories",
    { "--process-name", "myprocess", "--service-name", "myservice", "--label", "mylabel" });

  REQUIRE(histCliServer.paths.size() == 1);
  REQUIRE(histCliServer.paths[0] == "/history/myprocess/myservice/mylabel");
}

TEST_CASE("HistoryCliComponent: show_histories accepts the --flag=value form")
{
  histCliBackend histCliServer;
  auto histCliApi = std::make_shared<HistoryApi>(histCliProviderOf(histCliServer.port()));
  HistoryCliComponent histCliComponent(histCliApi);

  histCliCoutCapture histCliOut;
  histCliComponent.onCommand(UserDto{}, "show_histories", { "--process-name=onlyproc" });

  REQUIRE(histCliServer.paths.size() == 1);
  REQUIRE(histCliServer.paths[0] == "/history/onlyproc/.*/.*");
}

TEST_CASE("HistoryCliComponent: show_histories keeps the wildcard for filters that are not given")
{
  histCliBackend histCliServer;
  auto histCliApi = std::make_shared<HistoryApi>(histCliProviderOf(histCliServer.port()));
  HistoryCliComponent histCliComponent(histCliApi);

  histCliCoutCapture histCliOut;
  histCliComponent.onCommand(UserDto{}, "show_histories", { "-s", "myservice" });

  REQUIRE(histCliServer.paths.size() == 1);
  REQUIRE(histCliServer.paths[0] == "/history/.*/myservice/.*");
}

TEST_CASE("HistoryCliComponent: show_histories prints the entries returned by the api")
{
  histCliBackend histCliServer;
  histCliServer.answerWith(
    histCliOneEntryBody("returned-process", "returned-service", "returned-label", "returned-text"));
  auto histCliApi = std::make_shared<HistoryApi>(histCliProviderOf(histCliServer.port()));
  HistoryCliComponent histCliComponent(histCliApi);

  histCliCoutCapture histCliOut;
  histCliComponent.onCommand(UserDto{}, "show_histories", {});
  const std::string histCliPrinted = histCliOut.str();

  REQUIRE(histCliServer.paths.size() == 1);
  REQUIRE(histCliPrinted.find("returned-process") != std::string::npos);
  REQUIRE(histCliPrinted.find("returned-service") != std::string::npos);
  REQUIRE(histCliPrinted.find("returned-label") != std::string::npos);
  REQUIRE(histCliPrinted.find("returned-text") != std::string::npos);

  // the returned row is numbered 1 in the "No." column, so a counter that no longer starts
  // at 1 (or no longer increments) fails here
  const std::size_t histCliCell = histCliPrinted.find("returned-process");
  REQUIRE(histCliCell != std::string::npos);
  const std::size_t histCliLineStart = histCliPrinted.rfind('\n', histCliCell);
  REQUIRE(histCliLineStart != std::string::npos);
  REQUIRE(histCliPrinted.compare(histCliLineStart + 1, 3, "| 1") == 0);
}

TEST_CASE("HistoryCliComponent: an unknown command never reaches the api")
{
  histCliBackend histCliServer;
  auto histCliApi = std::make_shared<HistoryApi>(histCliProviderOf(histCliServer.port()));
  HistoryCliComponent histCliComponent(histCliApi);

  histCliCoutCapture histCliOut;
  histCliComponent.onCommand(UserDto{}, "this_command_does_not_exist", { "-p", "ignored" });
  const std::string histCliPrinted = histCliOut.str();

  REQUIRE(histCliServer.paths.empty());
  REQUIRE(histCliPrinted.empty());
}

TEST_CASE("HistoryCliComponent: show_histories without a value for -p prints an error")
{
  histCliBackend histCliServer;
  auto histCliApi = std::make_shared<HistoryApi>(histCliProviderOf(histCliServer.port()));
  HistoryCliComponent histCliComponent(histCliApi);

  histCliCerrCapture histCliErr;
  histCliComponent.onCommand(UserDto{}, "show_histories", { "-p" });
  const std::string histCliReported = histCliErr.str();

  REQUIRE(histCliServer.paths.empty());
  REQUIRE(histCliReported.find("ERROR") != std::string::npos);
}