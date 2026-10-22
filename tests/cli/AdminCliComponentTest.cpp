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

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/controller/UserDto.h"
#include "lintel/features/cli/components/AdminCliComponent.h"
#include "lintel/features/http/configuration/ClientConfiguration.h"
#include "lintel/features/http/configuration/HttpComponent.h"
#include "lintel/features/http/configuration/HttpEntry.h"
#include "lintel/features/http/provider/ClientProvider.h"

namespace lintel {
namespace {

  // `admCli` prefix: unity builds merge several test files into one anonymous
  // namespace, so a generic capture or backend name would collide.
  class admCliCoutCapture
  {
  public:
    admCliCoutCapture() : m_oldBuffer(std::cout.rdbuf(m_stream.rdbuf())) {}

    ~admCliCoutCapture() { std::cout.rdbuf(m_oldBuffer); }

    admCliCoutCapture(const admCliCoutCapture &) = delete;
    admCliCoutCapture &operator=(const admCliCoutCapture &) = delete;

    [[nodiscard]] std::string str() const { return m_stream.str(); }

  private:
    std::ostringstream m_stream;
    std::streambuf *m_oldBuffer;
  };

  class admCliCerrCapture
  {
  public:
    admCliCerrCapture() : m_oldBuffer(std::cerr.rdbuf(m_stream.rdbuf())) {}

    ~admCliCerrCapture() { std::cerr.rdbuf(m_oldBuffer); }

    admCliCerrCapture(const admCliCerrCapture &) = delete;
    admCliCerrCapture &operator=(const admCliCerrCapture &) = delete;

    [[nodiscard]] std::string str() const { return m_stream.str(); }

  private:
    std::ostringstream m_stream;
    std::streambuf *m_oldBuffer;
  };

  /**
   * `AdminApi` has no virtual seam, so the component is driven against a
   * loopback HTTP server that records every request and answers with a canned
   * body.
   */
  class admCliBackend
  {
  public:
    admCliBackend()
    {
      // Record in the route handlers, not in a pre-routing handler: the
      // pre-routing hook runs before httplib has read the request body, so it
      // only ever sees an empty request.body.
      const auto record = [this](const httplib::Request &request, httplib::Response &response) {
        paths.push_back(request.path);
        methods.emplace_back(request.method);
        bodies.push_back(request.body);
        response.status = m_status;
        response.set_content(m_responseBody, "application/json");
      };
      m_server.Get(".*", record);
      m_server.Post(".*", record);
      m_server.Put(".*", record);
      m_server.Delete(".*", record);
      m_port = m_server.bind_to_any_port("127.0.0.1");
      m_thread = std::thread([this]() { m_server.listen_after_bind(); });
      // Server::stop() is a no-op until the accept loop is up.
      const auto admCliDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
      while (!m_server.is_running() && std::chrono::steady_clock::now() < admCliDeadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }

    ~admCliBackend()
    {
      m_server.stop();
      if (m_thread.joinable()) { m_thread.join(); }
    }

    admCliBackend(const admCliBackend &) = delete;
    admCliBackend &operator=(const admCliBackend &) = delete;

    void answerWith(const std::string &body) { m_responseBody = body; }

    void setStatus(int status) { m_status = status; }

    [[nodiscard]] int port() const { return m_port; }

    [[nodiscard]] bool running() const { return m_server.is_running(); }

    std::vector<std::string> paths;
    std::vector<std::string> methods;
    std::vector<std::string> bodies;

  private:
    httplib::Server m_server;
    std::thread m_thread;
    int m_port{ -1 };
    std::string m_responseBody{ "{}" };
    int m_status{ 200 };
  };

  /** Client provider whose only client entry points at the loopback backend. */
  std::shared_ptr<ClientProvider> admCliProviderOf(int port)
  {
    static const std::string kAdmCliAbsentConfigDir = "/nonexistent/lintel-admin-cli-test-config";
    auto environment = std::make_shared<EnvironmentConfiguration>();
    environment->overrides(EnvironmentConfiguration::ConfigDirectory, kAdmCliAbsentConfigDir);
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

  /** @return the JSON body AdminController would send for a successful reload */
  std::string admCliReloadedBody() { return R"({"status":"reloaded","results":{"tls":"ok"}})"; }

}// namespace

TEST_CASE("AdminCliComponent: onShowMenu prints the no-submenu notice")
{
  auto admCliApi = std::make_shared<AdminApi>(admCliProviderOf(1));
  AdminCliComponent admCliComponent(admCliApi);

  admCliCoutCapture admCliOut;
  admCliComponent.onShowMenu();

  REQUIRE(admCliOut.str().find("No submenu available!") != std::string::npos);
}

TEST_CASE("AdminCliComponent: onMenu returns false for an unknown component")
{
  auto admCliApi = std::make_shared<AdminApi>(admCliProviderOf(1));
  AdminCliComponent admCliComponent(admCliApi);

  // CommandLineService only prints "Invalid command" when the component reports false.
  REQUIRE(admCliComponent.onMenu("History") == false);
}

TEST_CASE("AdminCliComponent: onHelp prints the header and every command with its arguments")
{
  auto admCliApi = std::make_shared<AdminApi>(admCliProviderOf(1));
  AdminCliComponent admCliComponent(admCliApi);

  admCliCoutCapture admCliOut;
  admCliComponent.onHelp();
  const std::string admCliHelp = admCliOut.str();

  REQUIRE(admCliHelp.find("Help Menu of Admin [Adm]:") != std::string::npos);
  REQUIRE(admCliHelp.find("reload_tls") != std::string::npos);
  REQUIRE(admCliHelp.find("log_level") != std::string::npos);
  REQUIRE(admCliHelp.find("--level, -l") != std::string::npos);
}

TEST_CASE("AdminCliComponent: printCommandList lists the registered commands")
{
  auto admCliApi = std::make_shared<AdminApi>(admCliProviderOf(1));
  AdminCliComponent admCliComponent(admCliApi);

  admCliCoutCapture admCliOut;
  admCliComponent.printCommandList({});
  const std::string admCliList = admCliOut.str();

  REQUIRE(admCliList.find("reload_tls") != std::string::npos);
  REQUIRE(admCliList.find("log_level") != std::string::npos);
}

TEST_CASE("AdminCliComponent: printCommandList appends the sub-menu aliases")
{
  auto admCliApi = std::make_shared<AdminApi>(admCliProviderOf(1));
  AdminCliComponent admCliComponent(admCliApi);

  admCliCoutCapture admCliOut;
  admCliComponent.printCommandList({ "exit", "help" });
  const std::string admCliList = admCliOut.str();

  REQUIRE(admCliList.find("reload_tls") != std::string::npos);
  REQUIRE(admCliList.find("exit") != std::string::npos);
  REQUIRE(admCliList.find("help") != std::string::npos);
}

TEST_CASE("AdminCliComponent: allCommandsOf returns the registered commands")
{
  auto admCliApi = std::make_shared<AdminApi>(admCliProviderOf(1));
  AdminCliComponent admCliComponent(admCliApi);

  const std::vector<std::string> admCliCommands = admCliComponent.allCommandsOf();
  const auto admCliHas = [&admCliCommands](const std::string &command) {
    return std::find(admCliCommands.begin(), admCliCommands.end(), command) != admCliCommands.end();
  };

  REQUIRE(admCliCommands.size() == 2);
  REQUIRE(admCliHas("reload_tls"));
  REQUIRE(admCliHas("log_level"));
}

TEST_CASE("AdminCliComponent: the component registers itself under name and alias")
{
  auto admCliApi = std::make_shared<AdminApi>(admCliProviderOf(1));
  AdminCliComponent admCliComponent(admCliApi);

  REQUIRE(admCliComponent.getName() == "Admin");
  REQUIRE(admCliComponent.getAlias() == "Adm");
  REQUIRE(admCliComponent.onExit() == true);
}

TEST_CASE("AdminCliComponent: reload_tls posts to the admin route and prints every resource")
{
  admCliBackend admCliServer;
  REQUIRE(admCliServer.port() > 0);
  REQUIRE(admCliServer.running());
  admCliServer.answerWith(admCliReloadedBody());
  auto admCliApi = std::make_shared<AdminApi>(admCliProviderOf(admCliServer.port()));
  AdminCliComponent admCliComponent(admCliApi);

  admCliCoutCapture admCliOut;
  admCliComponent.onCommand(UserDto{}, "reload_tls", {});
  const std::string admCliPrinted = admCliOut.str();

  REQUIRE(admCliServer.paths.size() == 1);
  REQUIRE(admCliServer.paths[0] == "/admin/reload-tls");
  REQUIRE(admCliServer.methods[0] == "POST");
  REQUIRE(admCliServer.bodies[0].empty());
  REQUIRE(admCliPrinted.find("tls: ok") != std::string::npos);
  REQUIRE(admCliPrinted.find("Reloaded the TLS certificate") != std::string::npos);
}

TEST_CASE("AdminCliComponent: reload_tls reports that there is nothing to reload")
{
  admCliBackend admCliServer;
  admCliServer.setStatus(409);
  admCliServer.answerWith(R"({"status":"nothing_to_reload","results":{}})");
  auto admCliApi = std::make_shared<AdminApi>(admCliProviderOf(admCliServer.port()));
  AdminCliComponent admCliComponent(admCliApi);

  admCliCoutCapture admCliOut;
  admCliComponent.onCommand(UserDto{}, "reload_tls", {});
  const std::string admCliPrinted = admCliOut.str();

  REQUIRE(admCliServer.paths.size() == 1);
  REQUIRE(admCliPrinted.find("ERROR") != std::string::npos);
  REQUIRE(admCliPrinted.find("nothing to reload") != std::string::npos);
}

TEST_CASE("AdminCliComponent: reload_tls prints the failure reason and the failed count")
{
  admCliBackend admCliServer;
  admCliServer.setStatus(500);
  admCliServer.answerWith(R"({"status":"failed","results":{"tls":"failed: key does not match","audit":"ok"}})");
  auto admCliApi = std::make_shared<AdminApi>(admCliProviderOf(admCliServer.port()));
  AdminCliComponent admCliComponent(admCliApi);

  admCliCoutCapture admCliOut;
  admCliComponent.onCommand(UserDto{}, "reload_tls", {});
  const std::string admCliPrinted = admCliOut.str();

  REQUIRE(admCliPrinted.find("tls: failed: key does not match") != std::string::npos);
  REQUIRE(admCliPrinted.find("audit: ok") != std::string::npos);
  REQUIRE(admCliPrinted.find("1 of 2") != std::string::npos);
}

TEST_CASE("AdminCliComponent: reload_tls reports an unauthorized token")
{
  admCliBackend admCliServer;
  admCliServer.setStatus(401);
  admCliServer.answerWith("");
  auto admCliApi = std::make_shared<AdminApi>(admCliProviderOf(admCliServer.port()));
  AdminCliComponent admCliComponent(admCliApi);

  admCliCerrCapture admCliErr;
  admCliComponent.onCommand(UserDto{}, "reload_tls", {});
  const std::string admCliReported = admCliErr.str();

  REQUIRE(admCliServer.paths.size() == 1);
  REQUIRE(admCliReported.find("ERROR") != std::string::npos);
  REQUIRE(admCliReported.find("Unauthorized") != std::string::npos);
}

TEST_CASE("AdminCliComponent: log_level posts the level and prints what the controller applied")
{
  admCliBackend admCliServer;
  admCliServer.answerWith(R"({"status":"changed","level":"debug"})");
  auto admCliApi = std::make_shared<AdminApi>(admCliProviderOf(admCliServer.port()));
  AdminCliComponent admCliComponent(admCliApi);

  admCliCoutCapture admCliOut;
  admCliComponent.onCommand(UserDto{}, "log_level", { "--level", "debug" });
  const std::string admCliPrinted = admCliOut.str();

  REQUIRE(admCliServer.paths.size() == 1);
  REQUIRE(admCliServer.paths[0] == "/admin/log-level");
  REQUIRE(admCliServer.methods[0] == "POST");
  REQUIRE(admCliServer.bodies[0] == R"({"level":"debug"})");
  REQUIRE(admCliPrinted.find("Log level changed to debug") != std::string::npos);
}

TEST_CASE("AdminCliComponent: log_level accepts the short flag")
{
  admCliBackend admCliServer;
  admCliServer.answerWith(R"({"status":"changed","level":"warn"})");
  auto admCliApi = std::make_shared<AdminApi>(admCliProviderOf(admCliServer.port()));
  AdminCliComponent admCliComponent(admCliApi);

  admCliCoutCapture admCliOut;
  admCliComponent.onCommand(UserDto{}, "log_level", { "-l", "warn" });

  REQUIRE(admCliServer.bodies[0] == R"({"level":"warn"})");
  REQUIRE(admCliOut.str().find("Log level changed to warn") != std::string::npos);
}

TEST_CASE("AdminCliComponent: log_level without a level prints an error and never reaches the api")
{
  admCliBackend admCliServer;
  auto admCliApi = std::make_shared<AdminApi>(admCliProviderOf(admCliServer.port()));
  AdminCliComponent admCliComponent(admCliApi);

  admCliCerrCapture admCliErr;
  admCliComponent.onCommand(UserDto{}, "log_level", {});
  const std::string admCliReported = admCliErr.str();

  REQUIRE(admCliServer.paths.empty());
  REQUIRE(admCliReported.find("ERROR") != std::string::npos);
  REQUIRE(admCliReported.find("--level") != std::string::npos);
}

TEST_CASE("AdminCliComponent: log_level reports a rejected level")
{
  admCliBackend admCliServer;
  admCliServer.setStatus(400);
  admCliServer.answerWith(R"({"status":"invalid_level","level":"verbose"})");
  auto admCliApi = std::make_shared<AdminApi>(admCliProviderOf(admCliServer.port()));
  AdminCliComponent admCliComponent(admCliApi);

  admCliCoutCapture admCliOut;
  admCliComponent.onCommand(UserDto{}, "log_level", { "-l", "verbose" });
  const std::string admCliPrinted = admCliOut.str();

  REQUIRE(admCliPrinted.find("ERROR") != std::string::npos);
  REQUIRE(admCliPrinted.find("verbose") != std::string::npos);
  REQUIRE(admCliPrinted.find("invalid_level") != std::string::npos);
}

TEST_CASE("AdminCliComponent: an unknown command never reaches the api")
{
  admCliBackend admCliServer;
  auto admCliApi = std::make_shared<AdminApi>(admCliProviderOf(admCliServer.port()));
  AdminCliComponent admCliComponent(admCliApi);

  admCliCoutCapture admCliOut;
  admCliComponent.onCommand(UserDto{}, "reload_certificate", { "-l", "ignored" });

  REQUIRE(admCliServer.paths.empty());
  REQUIRE(admCliOut.str().empty());
}
}// namespace lintel
