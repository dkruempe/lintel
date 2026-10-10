#include <catch2/catch_all.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
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
#include "lintel/features/base/controller/ProcessGroupDto.h"
#include "lintel/features/base/controller/ProcessGroupsDto.h"
#include "lintel/features/base/controller/ProcessInfoDto.h"
#include "lintel/features/base/controller/ProcessInfosDto.h"
#include "lintel/features/base/controller/UserDto.h"
#include "lintel/features/base/models/Process.h"
#include "lintel/features/base/models/ProcessGroup.h"
#include "lintel/features/base/models/ProcessInfo.h"
#include "lintel/features/cli/components/ProcessCliComponent.h"
#include "lintel/features/http/configuration/ClientConfiguration.h"
#include "lintel/features/http/configuration/HttpComponent.h"
#include "lintel/features/http/configuration/HttpEntry.h"
#include "lintel/features/http/provider/ClientProvider.h"

namespace {

/** RAII redirector for `std::cout`, restored on destruction even when a REQUIRE throws. */
class procCliCoutCapture
{
public:
  procCliCoutCapture() : m_oldBuffer(std::cout.rdbuf(m_stream.rdbuf())) {}

  ~procCliCoutCapture() { std::cout.rdbuf(m_oldBuffer); }

  procCliCoutCapture(const procCliCoutCapture &) = delete;
  procCliCoutCapture &operator=(const procCliCoutCapture &) = delete;

  [[nodiscard]] std::string str() const { return m_stream.str(); }

private:
  std::ostringstream m_stream;
  std::streambuf *m_oldBuffer;
};

/** RAII redirector for `std::cerr`, used for the error paths of onCommand(). */
class procCliCerrCapture
{
public:
  procCliCerrCapture() : m_oldBuffer(std::cerr.rdbuf(m_stream.rdbuf())) {}

  ~procCliCerrCapture() { std::cerr.rdbuf(m_oldBuffer); }

  procCliCerrCapture(const procCliCerrCapture &) = delete;
  procCliCerrCapture &operator=(const procCliCerrCapture &) = delete;

  [[nodiscard]] std::string str() const { return m_stream.str(); }

private:
  std::ostringstream m_stream;
  std::streambuf *m_oldBuffer;
};

/**
 * `ProcessApi` has no virtual seam, so the component is driven against a loopback HTTP server
 * that records method, path and body of every request and answers with a canned body.
 */
class procCliBackend
{
public:
  procCliBackend()
  {
    m_server.set_pre_routing_handler([this](const httplib::Request &request, httplib::Response &response) {
      paths.push_back(request.path);
      methods.emplace_back(request.method);
      bodies.emplace_back();
      if (request.method == "POST" || request.method == "PUT") {
        // the request body is only read once a route matched, so let routing
        // continue and let the catch-all handlers below serve the answer
        return httplib::Server::HandlerResponse::Unhandled;
      }
      response.status = m_responseStatus;
      response.set_content(m_responseBody, "application/json");
      return httplib::Server::HandlerResponse::Handled;
    });
    const auto procCliAnswer = [this](const httplib::Request &request, httplib::Response &response) {
      if (!bodies.empty()) { bodies.back() = request.body; }
      response.status = m_responseStatus;
      response.set_content(m_responseBody, "application/json");
    };
    m_server.Post(R"(.*)", procCliAnswer);
    m_server.Put(R"(.*)", procCliAnswer);
    m_port = m_server.bind_to_any_port("127.0.0.1");
    m_thread = std::thread([this]() { m_server.listen_after_bind(); });
    // Server::stop() is a no-op until the accept loop is up. Without this wait a
    // backend that is created and destroyed without a single request leaves the
    // accept loop blocked in accept() and the join in the destructor never returns.
    const auto procCliDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!m_server.is_running() && std::chrono::steady_clock::now() < procCliDeadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  ~procCliBackend()
  {
    m_server.stop();
    if (m_thread.joinable()) { m_thread.join(); }
  }

  procCliBackend(const procCliBackend &) = delete;
  procCliBackend &operator=(const procCliBackend &) = delete;

  void answerWith(const std::string &body)
  {
    m_responseBody = body;
    m_responseStatus = 200;
  }

  void answerWithStatus(int status, const std::string &body)
  {
    m_responseBody = body;
    m_responseStatus = status;
  }

  [[nodiscard]] int port() const { return m_port; }

  /** @return true once the accept loop is up, so stop() can shut the server down */
  [[nodiscard]] bool running() const { return m_server.is_running(); }

  std::vector<std::string> paths;
  std::vector<std::string> methods;
  std::vector<std::string> bodies;

private:
  httplib::Server m_server;
  std::thread m_thread;
  int m_port{ -1 };
  std::string m_responseBody{ "[]" };
  int m_responseStatus{ 200 };
};

/** Client provider whose only client entry points at the loopback backend. */
std::shared_ptr<ClientProvider> procCliProviderOf(int port)
{
  static const std::string kProcCliAbsentConfigDir = "/nonexistent/lintel-proc-cli-test-config";
  auto environment = std::make_shared<EnvironmentConfiguration>();
  environment->overrides(EnvironmentConfiguration::ConfigDirectory, kProcCliAbsentConfigDir);
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

/** @return the JSON body ProcessController would send for a list of processes */
std::string procCliOneProcessBody(const std::string &executable,
  const std::string &groupName,
  const std::string &groupId,
  pid_t systemPid)
{
  auto procCliProcess = std::make_shared<Process>(std::filesystem::path(executable), std::vector<std::string>{});
  ProcessInfo procCliInfo(procCliProcess, systemPid, true, 0, groupName, groupId);
  ProcessInfosDto procCliDtos(std::vector<ProcessInfo>{ procCliInfo });
  return procCliDtos.JsonSerializable::serialize();
}

/** @return the JSON body of the process-group endpoint */
std::string procCliOneGroupBody(const std::string &groupName)
{
  ProcessGroup procCliGroup(groupName, std::vector<Process>{});
  ProcessGroupDto procCliGroupDto(std::make_shared<ProcessGroup>(procCliGroup), std::vector<ProcessInfo>{});
  ProcessGroupsDto procCliGroupsDto(std::vector<ProcessGroupDto>{ procCliGroupDto });
  return procCliGroupsDto.JsonSerializable::serialize();
}

/** @return the JSON body of the health endpoint for a single process */
std::string procCliHealthBody(const std::string &executable,
  const std::string &groupName,
  const std::string &groupId,
  pid_t systemPid)
{
  auto procCliProcess = std::make_shared<Process>(std::filesystem::path(executable), std::vector<std::string>{});
  ProcessInfo procCliInfo(procCliProcess, systemPid, true, 0, groupName, groupId);
  ProcessInfoDto procCliInfoDto(procCliInfo);
  return procCliInfoDto.JsonSerializable::serialize();
}

}// namespace

TEST_CASE("ProcessCliComponent: onShowMenu prints the no-submenu notice")
{
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(1));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onShowMenu();

  REQUIRE(procCliOut.str().find("No subMenu available") != std::string::npos);
}

TEST_CASE("ProcessCliComponent: onMenu returns false for an unknown component")
{
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(1));
  ProcessCliComponent procCliComponent(procCliApi);

  // CommandLineService only prints "Invalid command" when the component reports false.
  REQUIRE(procCliComponent.onMenu("History") == false);
}

TEST_CASE("ProcessCliComponent: onMenu returns false for its own alias")
{
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(1));
  ProcessCliComponent procCliComponent(procCliApi);

  const bool procCliAccepted = procCliComponent.onMenu(std::string(procCliComponent.getAlias()));
  REQUIRE(procCliAccepted == false);
}

TEST_CASE("ProcessCliComponent: onHelp prints the header and every command with its arguments")
{
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(1));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onHelp();
  const std::string procCliHelp = procCliOut.str();

  REQUIRE(procCliHelp.find("Help Menu of Process [Pros]:") != std::string::npos);
  REQUIRE(procCliHelp.find("show_processes") != std::string::npos);
  REQUIRE(procCliHelp.find("show_groups") != std::string::npos);
  REQUIRE(procCliHelp.find("start_process") != std::string::npos);
  REQUIRE(procCliHelp.find("stop_process") != std::string::npos);
  REQUIRE(procCliHelp.find("terminate_process") != std::string::npos);
  REQUIRE(procCliHelp.find("restart_process") != std::string::npos);
  REQUIRE(procCliHelp.find("reset_process") != std::string::npos);
  REQUIRE(procCliHelp.find("show_process_details") != std::string::npos);
}

TEST_CASE("ProcessCliComponent: printCommandList lists every registered command")
{
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(1));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.printCommandList({});
  const std::string procCliList = procCliOut.str();

  REQUIRE(procCliList.find("show_processes") != std::string::npos);
  REQUIRE(procCliList.find("show_process_details") != std::string::npos);
  REQUIRE(procCliList.find("restart_process") != std::string::npos);
}

TEST_CASE("ProcessCliComponent: printCommandList appends the sub-menu aliases")
{
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(1));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.printCommandList({ "exit" });
  const std::string procCliList = procCliOut.str();

  REQUIRE(procCliList.find("show_processes") != std::string::npos);
  REQUIRE(procCliList.find("exit") != std::string::npos);
}

TEST_CASE("ProcessCliComponent: allCommandsOf returns the eight registered commands")
{
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(1));
  ProcessCliComponent procCliComponent(procCliApi);

  const std::vector<std::string> procCliCommands = procCliComponent.allCommandsOf();
  bool procCliHasAll = true;
  for (const char *procCliExpected : { "show_processes",
         "show_groups",
         "start_process",
         "stop_process",
         "terminate_process",
         "restart_process",
         "reset_process",
         "show_process_details" }) {
    procCliHasAll =
      procCliHasAll
      && std::find(procCliCommands.begin(), procCliCommands.end(), procCliExpected) != procCliCommands.end();
  }

  REQUIRE(procCliCommands.size() == 8);
  REQUIRE(procCliHasAll);
}

TEST_CASE("ProcessCliComponent: the component registers itself under name and alias")
{
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(1));
  ProcessCliComponent procCliComponent(procCliApi);

  REQUIRE(procCliComponent.getName() == "Process");
  REQUIRE(procCliComponent.getAlias() == "Pros");
}

TEST_CASE("ProcessCliComponent: onExit confirms the exit")
{
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(1));
  ProcessCliComponent procCliComponent(procCliApi);

  REQUIRE(procCliComponent.onExit() == true);
}

TEST_CASE("ProcessCliComponent: show_processes without flags asks the api for every process")
{
  procCliBackend procCliServer;
  REQUIRE(procCliServer.port() > 0);
  REQUIRE(procCliServer.running());
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "show_processes", {});
  const std::string procCliPrinted = procCliOut.str();

  REQUIRE(procCliServer.paths.size() == 1);
  REQUIRE(procCliServer.paths[0] == "/process/processes/.*");
  REQUIRE(procCliServer.methods[0] == "GET");
  REQUIRE(procCliPrinted.find("GroupName") != std::string::npos);
}

TEST_CASE("ProcessCliComponent: show_processes -p forwards the process name filter")
{
  procCliBackend procCliServer;
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "show_processes", { "--process-name", "onlyproc" });

  REQUIRE(procCliServer.paths.size() == 1);
  REQUIRE(procCliServer.paths[0] == "/process/processes/onlyproc");
}

TEST_CASE("ProcessCliComponent: show_processes prints the processes returned by the api")
{
  procCliBackend procCliServer;
  procCliServer.answerWith(procCliOneProcessBody("/opt/lintel/myworker", "worker-group", "group-uuid", 4242));
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "show_processes", {});
  const std::string procCliPrinted = procCliOut.str();

  REQUIRE(procCliServer.paths.size() == 1);
  REQUIRE(procCliPrinted.find("myworker") != std::string::npos);
  REQUIRE(procCliPrinted.find("worker-group") != std::string::npos);
  REQUIRE(procCliPrinted.find("group-uuid") != std::string::npos);
  REQUIRE(procCliPrinted.find("4242") != std::string::npos);
}

TEST_CASE("ProcessCliComponent: show_groups without flags asks the api for every group")
{
  procCliBackend procCliServer;
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "show_groups", {});
  const std::string procCliPrinted = procCliOut.str();

  REQUIRE(procCliServer.paths.size() == 1);
  REQUIRE(procCliServer.paths[0] == "/process/groups/.*");
  REQUIRE(procCliServer.methods[0] == "GET");
  REQUIRE(procCliPrinted.find("ProcessGroupName") != std::string::npos);
}

TEST_CASE("ProcessCliComponent: show_groups -g forwards the group name filter")
{
  procCliBackend procCliServer;
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "show_groups", { "-g", "onlygroup" });

  REQUIRE(procCliServer.paths.size() == 1);
  REQUIRE(procCliServer.paths[0] == "/process/groups/onlygroup");
}

TEST_CASE("ProcessCliComponent: show_groups prints the groups returned by the api")
{
  procCliBackend procCliServer;
  procCliServer.answerWith(procCliOneGroupBody("returned-group"));
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "show_groups", {});
  const std::string procCliPrinted = procCliOut.str();

  REQUIRE(procCliServer.paths.size() == 1);
  REQUIRE(procCliPrinted.find("returned-group") != std::string::npos);
}

TEST_CASE("ProcessCliComponent: start_process without a path prints an error and sends nothing")
{
  procCliBackend procCliServer;
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "start_process", {});
  const std::string procCliPrinted = procCliOut.str();

  REQUIRE(procCliServer.paths.empty());
  REQUIRE(procCliPrinted.find("ERROR") != std::string::npos);
  REQUIRE(procCliPrinted.find("please enter path") != std::string::npos);
}

TEST_CASE("ProcessCliComponent: start_process posts the process path to the api")
{
  procCliBackend procCliServer;
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "start_process", { "-p", "/opt/lintel/worker" });
  const std::string procCliBody = procCliServer.bodies.empty() ? std::string{} : procCliServer.bodies[0];

  REQUIRE(procCliServer.paths.size() == 1);
  REQUIRE(procCliServer.paths[0] == "/process/start");
  REQUIRE(procCliServer.methods[0] == "POST");
  REQUIRE(procCliBody.find("/opt/lintel/worker") != std::string::npos);
}

TEST_CASE("ProcessCliComponent: start_process forwards the restart limit in the request body")
{
  procCliBackend procCliServer;
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "start_process", { "--process-path", "/opt/lintel/worker", "--restarts", "5" });
  const std::string procCliBody = procCliServer.bodies.empty() ? std::string{} : procCliServer.bodies[0];

  REQUIRE(procCliServer.paths.size() == 1);
  REQUIRE(procCliBody.find("\"auto_restart\":true") != std::string::npos);
  REQUIRE(procCliBody.find("\"max_restarts\":5") != std::string::npos);
}

TEST_CASE("ProcessCliComponent: stop_process without an id prints an error and sends nothing")
{
  procCliBackend procCliServer;
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "stop_process", {});
  const std::string procCliPrinted = procCliOut.str();

  REQUIRE(procCliServer.paths.empty());
  REQUIRE(procCliPrinted.find("please enter valid process id") != std::string::npos);
}

TEST_CASE("ProcessCliComponent: stop_process deletes the process by id")
{
  procCliBackend procCliServer;
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "stop_process", { "-i", "abc-123" });

  REQUIRE(procCliServer.paths.size() == 1);
  REQUIRE(procCliServer.paths[0] == "/process/stop/abc-123");
  REQUIRE(procCliServer.methods[0] == "DELETE");
}

TEST_CASE("ProcessCliComponent: terminate_process deletes the process by id")
{
  procCliBackend procCliServer;
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "terminate_process", { "--process-id", "term-9" });

  REQUIRE(procCliServer.paths.size() == 1);
  REQUIRE(procCliServer.paths[0] == "/process/terminate/term-9");
  REQUIRE(procCliServer.methods[0] == "DELETE");
}

TEST_CASE("ProcessCliComponent: restart_process puts the process by id")
{
  procCliBackend procCliServer;
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "restart_process", { "-i", "restart-1" });

  REQUIRE(procCliServer.paths.size() == 1);
  REQUIRE(procCliServer.paths[0] == "/process/restart/restart-1");
  REQUIRE(procCliServer.methods[0] == "PUT");
}

TEST_CASE("ProcessCliComponent: reset_process posts to the reset endpoint")
{
  procCliBackend procCliServer;
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "reset_process", { "-i", "reset-1" });

  REQUIRE(procCliServer.paths.size() == 1);
  REQUIRE(procCliServer.paths[0] == "/process/reset/reset-1");
  REQUIRE(procCliServer.methods[0] == "POST");
}

TEST_CASE("ProcessCliComponent: show_process_details without an id prints an error")
{
  procCliBackend procCliServer;
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "show_process_details", {});
  const std::string procCliPrinted = procCliOut.str();

  REQUIRE(procCliServer.paths.empty());
  REQUIRE(procCliPrinted.find("please enter valid process id") != std::string::npos);
}

TEST_CASE("ProcessCliComponent: show_process_details prints the health snapshot of the process")
{
  procCliBackend procCliServer;
  procCliServer.answerWith(procCliHealthBody("/opt/lintel/detailproc", "detail-group", "detail-uuid", 777));
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "show_process_details", { "-i", "detail-1" });
  const std::string procCliPrinted = procCliOut.str();

  REQUIRE(procCliServer.paths.size() == 1);
  REQUIRE(procCliServer.paths[0] == "/process/health/detail-1");
  REQUIRE(procCliServer.methods[0] == "GET");
  REQUIRE(procCliPrinted.find("detailproc") != std::string::npos);
  REQUIRE(procCliPrinted.find("detail-group") != std::string::npos);
  REQUIRE(procCliPrinted.find("Running") != std::string::npos);
}

TEST_CASE("ProcessCliComponent: show_process_details reports when no details are available")
{
  procCliBackend procCliServer;
  // a non-OK answer makes ProcessApi::healthOf return an empty optional
  procCliServer.answerWithStatus(404, "");
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "show_process_details", { "-i", "gone-1" });
  const std::string procCliPrinted = procCliOut.str();

  REQUIRE(procCliServer.paths.size() == 1);
  REQUIRE(procCliPrinted.find("no details available for process gone-1") != std::string::npos);
}

TEST_CASE("ProcessCliComponent: an unknown command never reaches the api")
{
  procCliBackend procCliServer;
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCoutCapture procCliOut;
  procCliComponent.onCommand(UserDto{}, "show_everything", { "-p", "ignored" });
  const std::string procCliPrinted = procCliOut.str();

  REQUIRE(procCliServer.paths.empty());
  REQUIRE(procCliPrinted.empty());
}

TEST_CASE("ProcessCliComponent: show_processes without a value for -p prints an error")
{
  procCliBackend procCliServer;
  auto procCliApi = std::make_shared<ProcessApi>(procCliProviderOf(procCliServer.port()));
  ProcessCliComponent procCliComponent(procCliApi);

  procCliCerrCapture procCliErr;
  procCliComponent.onCommand(UserDto{}, "show_processes", { "-p" });
  const std::string procCliReported = procCliErr.str();

  REQUIRE(procCliServer.paths.empty());
  REQUIRE(procCliReported.find("ERROR") != std::string::npos);
}