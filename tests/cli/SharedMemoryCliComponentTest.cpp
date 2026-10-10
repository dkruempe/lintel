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
#include "lintel/features/base/controller/UserDto.h"
#include "lintel/features/cli/components/SharedMemoryCliComponent.h"
#include "lintel/features/http/configuration/ClientConfiguration.h"
#include "lintel/features/http/configuration/HttpComponent.h"
#include "lintel/features/http/configuration/HttpEntry.h"
#include "lintel/features/http/provider/ClientProvider.h"

namespace {

/** RAII redirector for `std::cout`, restored on destruction even when a REQUIRE throws. */
class shmCliCoutCapture
{
public:
  shmCliCoutCapture() : m_oldBuffer(std::cout.rdbuf(m_stream.rdbuf())) {}

  ~shmCliCoutCapture() { std::cout.rdbuf(m_oldBuffer); }

  shmCliCoutCapture(const shmCliCoutCapture &) = delete;
  shmCliCoutCapture &operator=(const shmCliCoutCapture &) = delete;

  [[nodiscard]] std::string str() const { return m_stream.str(); }

private:
  std::ostringstream m_stream;
  std::streambuf *m_oldBuffer;
};

/** RAII redirector for `std::cerr`, used for the error paths of onCommand(). */
class shmCliCerrCapture
{
public:
  shmCliCerrCapture() : m_oldBuffer(std::cerr.rdbuf(m_stream.rdbuf())) {}

  ~shmCliCerrCapture() { std::cerr.rdbuf(m_oldBuffer); }

  shmCliCerrCapture(const shmCliCerrCapture &) = delete;
  shmCliCerrCapture &operator=(const shmCliCerrCapture &) = delete;

  [[nodiscard]] std::string str() const { return m_stream.str(); }

private:
  std::ostringstream m_stream;
  std::streambuf *m_oldBuffer;
};

/**
 * `SharedMemoryApi` has no virtual seam, so the component is driven against a loopback HTTP
 * server that records method and path of every request and answers with a canned body.
 */
class shmCliBackend
{
public:
  shmCliBackend()
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
    const auto shmCliDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!m_server.is_running() && std::chrono::steady_clock::now() < shmCliDeadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  ~shmCliBackend()
  {
    m_server.stop();
    if (m_thread.joinable()) { m_thread.join(); }
  }

  shmCliBackend(const shmCliBackend &) = delete;
  shmCliBackend &operator=(const shmCliBackend &) = delete;

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
std::shared_ptr<ClientProvider> shmCliProviderOf(int port)
{
  static const std::string kShmCliAbsentConfigDir = "/nonexistent/lintel-shm-cli-test-config";
  auto environment = std::make_shared<EnvironmentConfiguration>();
  environment->overrides(EnvironmentConfiguration::ConfigDirectory, kShmCliAbsentConfigDir);
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

/** @return a segment payload in the wire format of SharedMemorySegmentDto */
std::string
  shmCliOneSegmentBody(const std::string &name, const std::string &sizeInByte, const std::string &currentSizeInByte)
{
  return std::string("[{\"path\":\"/dev/shm/") + name + "\",\"name\":\"" + name + "\",\"size\":\"" + sizeInByte
         + "\",\"auto_extend\":false,\"auto_extend_size\":\"0\"," + "\"max_size\":\"" + sizeInByte
         + "\",\"sanity\":true,\"current_size\":\"" + currentSizeInByte
         + "\",\"free_size\":\"0\",\"named_objects\":\"3\"," + "\"unique_objects\":\"4\"}]";
}

/** @return a repository payload in the wire format of SharedMemoryRepositoryDto */
std::string shmCliOneRepositoryBody(const std::string &uuid,
  const std::string &name,
  const std::string &segmentName,
  const std::string &sizeInByte)
{
  return std::string("[{\"uuid\":\"") + uuid + "\",\"name\":\"" + name + "\",\"segment_name\":\"" + segmentName
         + "\",\"type\":\"Map\",\"size\":\"" + sizeInByte + "\",\"version\":7}]";
}

}// namespace

TEST_CASE("SharedMemoryCliComponent: onShowMenu prints the no-submenu notice")
{
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(1));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.onShowMenu();

  REQUIRE(shmCliOut.str().find("No subMenu available") != std::string::npos);
}

TEST_CASE("SharedMemoryCliComponent: onMenu returns false for an unknown component")
{
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(1));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  // CommandLineService only prints "Invalid command" when the component reports false.
  REQUIRE(shmCliComponent.onMenu("Process") == false);
}

TEST_CASE("SharedMemoryCliComponent: onMenu returns false for its own alias")
{
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(1));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  const bool shmCliAccepted = shmCliComponent.onMenu(std::string(shmCliComponent.getAlias()));
  REQUIRE(shmCliAccepted == false);
}

TEST_CASE("SharedMemoryCliComponent: onHelp prints the header and every command with its arguments")
{
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(1));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.onHelp();
  const std::string shmCliHelp = shmCliOut.str();

  REQUIRE(shmCliHelp.find("Help Menu of SharedMemory [Shm]:") != std::string::npos);
  REQUIRE(shmCliHelp.find("show_segments") != std::string::npos);
  REQUIRE(shmCliHelp.find("show_repositories") != std::string::npos);
  REQUIRE(shmCliHelp.find("shrink_segment") != std::string::npos);
  REQUIRE(shmCliHelp.find("grow_segment") != std::string::npos);
  REQUIRE(shmCliHelp.find("export_repository") != std::string::npos);
  REQUIRE(shmCliHelp.find("--segment-name, -s") != std::string::npos);
  REQUIRE(shmCliHelp.find("--grow-size, -g") != std::string::npos);
  REQUIRE(shmCliHelp.find("--uuid, -u") != std::string::npos);
}

TEST_CASE("SharedMemoryCliComponent: printCommandList lists every registered command")
{
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(1));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.printCommandList({});
  const std::string shmCliList = shmCliOut.str();

  REQUIRE(shmCliList.find("show_segments") != std::string::npos);
  REQUIRE(shmCliList.find("show_repositories") != std::string::npos);
  REQUIRE(shmCliList.find("shrink_segment") != std::string::npos);
  REQUIRE(shmCliList.find("grow_segment") != std::string::npos);
  REQUIRE(shmCliList.find("export_repository") != std::string::npos);
}

TEST_CASE("SharedMemoryCliComponent: printCommandList appends the sub-menu aliases")
{
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(1));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.printCommandList({ "exit", "help" });
  const std::string shmCliList = shmCliOut.str();

  REQUIRE(shmCliList.find("show_segments") != std::string::npos);
  REQUIRE(shmCliList.find("exit") != std::string::npos);
  REQUIRE(shmCliList.find("help") != std::string::npos);
}

TEST_CASE("SharedMemoryCliComponent: allCommandsOf returns the five registered commands")
{
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(1));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  const std::vector<std::string> shmCliCommands = shmCliComponent.allCommandsOf();
  bool shmCliHasAll = true;
  for (const char *shmCliExpected :
    { "show_segments", "show_repositories", "shrink_segment", "grow_segment", "export_repository" }) {
    shmCliHasAll =
      shmCliHasAll && std::find(shmCliCommands.begin(), shmCliCommands.end(), shmCliExpected) != shmCliCommands.end();
  }

  REQUIRE(shmCliCommands.size() == 5);
  REQUIRE(shmCliHasAll);
}

TEST_CASE("SharedMemoryCliComponent: the component registers itself under name and alias")
{
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(1));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  REQUIRE(shmCliComponent.getName() == "SharedMemory");
  REQUIRE(shmCliComponent.getAlias() == "Shm");
}

TEST_CASE("SharedMemoryCliComponent: onExit confirms the exit")
{
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(1));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  REQUIRE(shmCliComponent.onExit() == true);
}

TEST_CASE("SharedMemoryCliComponent: show_segments without flags asks for every segment")
{
  shmCliBackend shmCliServer;
  REQUIRE(shmCliServer.port() > 0);
  REQUIRE(shmCliServer.running());
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(shmCliServer.port()));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.onCommand(UserDto{}, "show_segments", {});
  const std::string shmCliPrinted = shmCliOut.str();

  REQUIRE(shmCliServer.paths.size() == 1);
  REQUIRE(shmCliServer.paths[0] == "/shm/segments/.*");
  REQUIRE(shmCliServer.methods[0] == "GET");
  REQUIRE(shmCliPrinted.find("AutoExtendSize") != std::string::npos);
}

TEST_CASE("SharedMemoryCliComponent: show_segments -s forwards the segment name filter")
{
  shmCliBackend shmCliServer;
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(shmCliServer.port()));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.onCommand(UserDto{}, "show_segments", { "--segment-name", "onlysegment" });

  REQUIRE(shmCliServer.paths.size() == 1);
  REQUIRE(shmCliServer.paths[0] == "/shm/segments/onlysegment");
}

TEST_CASE("SharedMemoryCliComponent: show_segments prints the segments returned by the api")
{
  shmCliBackend shmCliServer;
  shmCliServer.answerWith(shmCliOneSegmentBody("returned-segment", "4000", "2000"));
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(shmCliServer.port()));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.onCommand(UserDto{}, "show_segments", {});
  const std::string shmCliPrinted = shmCliOut.str();

  REQUIRE(shmCliServer.paths.size() == 1);
  REQUIRE(shmCliPrinted.find("returned-segment") != std::string::npos);
  // the size columns are human readable: MemorySize::serialize turns the raw byte counts
  // into 4KB / 2KB
  REQUIRE(shmCliPrinted.find("4KB") != std::string::npos);
  REQUIRE(shmCliPrinted.find("2KB") != std::string::npos);
  // NamedObjects / UniqueObjects columns carry the raw counters
  REQUIRE(shmCliPrinted.find("3") != std::string::npos);
  REQUIRE(shmCliPrinted.find("4") != std::string::npos);
}

TEST_CASE("SharedMemoryCliComponent: show_repositories without flags asks for every repository")
{
  shmCliBackend shmCliServer;
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(shmCliServer.port()));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.onCommand(UserDto{}, "show_repositories", {});
  const std::string shmCliPrinted = shmCliOut.str();

  REQUIRE(shmCliServer.paths.size() == 1);
  REQUIRE(shmCliServer.paths[0] == "/shm/repositories/.*/.*");
  REQUIRE(shmCliServer.methods[0] == "GET");
  REQUIRE(shmCliPrinted.find("RepositoryName") != std::string::npos);
}

TEST_CASE("SharedMemoryCliComponent: show_repositories forwards both filters")
{
  shmCliBackend shmCliServer;
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(shmCliServer.port()));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.onCommand(UserDto{}, "show_repositories", { "-r", "onlyrepo", "-s", "onlysegment" });

  REQUIRE(shmCliServer.paths.size() == 1);
  REQUIRE(shmCliServer.paths[0] == "/shm/repositories/onlyrepo/onlysegment");
}

TEST_CASE("SharedMemoryCliComponent: show_repositories prints the repositories returned by the api")
{
  shmCliBackend shmCliServer;
  shmCliServer.answerWith(shmCliOneRepositoryBody("repo-uuid", "returned-repo", "returned-seg", "64"));
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(shmCliServer.port()));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.onCommand(UserDto{}, "show_repositories", {});
  const std::string shmCliPrinted = shmCliOut.str();

  REQUIRE(shmCliServer.paths.size() == 1);
  REQUIRE(shmCliPrinted.find("repo-uuid") != std::string::npos);
  REQUIRE(shmCliPrinted.find("returned-repo") != std::string::npos);
  REQUIRE(shmCliPrinted.find("returned-seg") != std::string::npos);
  REQUIRE(shmCliPrinted.find("Map") != std::string::npos);
  REQUIRE(shmCliPrinted.find("7") != std::string::npos);
}

TEST_CASE("SharedMemoryCliComponent: shrink_segment without a segment name prints an error")
{
  shmCliBackend shmCliServer;
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(shmCliServer.port()));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.onCommand(UserDto{}, "shrink_segment", {});
  const std::string shmCliPrinted = shmCliOut.str();

  REQUIRE(shmCliServer.paths.empty());
  REQUIRE(shmCliPrinted.find("ERROR") != std::string::npos);
  REQUIRE(shmCliPrinted.find("please set segment name") != std::string::npos);
}

TEST_CASE("SharedMemoryCliComponent: shrink_segment puts the segment name to the api")
{
  shmCliBackend shmCliServer;
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(shmCliServer.port()));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.onCommand(UserDto{}, "shrink_segment", { "-s", "myshsegment" });

  REQUIRE(shmCliServer.paths.size() == 1);
  REQUIRE(shmCliServer.paths[0] == "/shm/segments/shrink/myshsegment");
  REQUIRE(shmCliServer.methods[0] == "PUT");
}

TEST_CASE("SharedMemoryCliComponent: grow_segment without a segment name prints an error")
{
  shmCliBackend shmCliServer;
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(shmCliServer.port()));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.onCommand(UserDto{}, "grow_segment", { "-g", "4MB" });
  const std::string shmCliPrinted = shmCliOut.str();

  REQUIRE(shmCliServer.paths.empty());
  REQUIRE(shmCliPrinted.find("please set segment name") != std::string::npos);
}

TEST_CASE("SharedMemoryCliComponent: grow_segment without a grow size prints an error")
{
  shmCliBackend shmCliServer;
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(shmCliServer.port()));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.onCommand(UserDto{}, "grow_segment", { "-s", "myshsegment" });
  const std::string shmCliPrinted = shmCliOut.str();

  REQUIRE(shmCliServer.paths.empty());
  REQUIRE(shmCliPrinted.find("ERROR") != std::string::npos);
  REQUIRE(shmCliPrinted.find("please set grow size") != std::string::npos);
}

TEST_CASE("SharedMemoryCliComponent: grow_segment puts segment name and size to the api")
{
  shmCliBackend shmCliServer;
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(shmCliServer.port()));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.onCommand(UserDto{}, "grow_segment", { "--segment-name", "myshsegment", "-g", "4MB" });

  REQUIRE(shmCliServer.paths.size() == 1);
  REQUIRE(shmCliServer.paths[0] == "/shm/segments/grow/myshsegment/4MB");
  REQUIRE(shmCliServer.methods[0] == "PUT");
}

TEST_CASE("SharedMemoryCliComponent: export_repository without a uuid prints an error")
{
  shmCliBackend shmCliServer;
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(shmCliServer.port()));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.onCommand(UserDto{}, "export_repository", { "-r", "someRepo" });
  const std::string shmCliPrinted = shmCliOut.str();

  REQUIRE(shmCliServer.paths.empty());
  REQUIRE(shmCliPrinted.find("ERROR") != std::string::npos);
  REQUIRE(shmCliPrinted.find("please set uuid") != std::string::npos);
}

TEST_CASE("SharedMemoryCliComponent: export_repository asks the api for the uuid")
{
  shmCliBackend shmCliServer;
  shmCliServer.answerWith("{\"exported\":1}");
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(shmCliServer.port()));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.onCommand(UserDto{}, "export_repository", { "--uuid", "export-uuid" });
  const std::string shmCliPrinted = shmCliOut.str();

  REQUIRE(shmCliServer.paths.size() == 1);
  REQUIRE(shmCliServer.paths[0] == "/shm/repository/export-uuid");
  REQUIRE(shmCliServer.methods[0] == "GET");
  REQUIRE(shmCliPrinted.find("exported") != std::string::npos);
}

TEST_CASE("SharedMemoryCliComponent: an unknown command never reaches the api")
{
  shmCliBackend shmCliServer;
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(shmCliServer.port()));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCoutCapture shmCliOut;
  shmCliComponent.onCommand(UserDto{}, "show_segments_of_everything", { "-s", "ignored" });
  const std::string shmCliPrinted = shmCliOut.str();

  REQUIRE(shmCliServer.paths.empty());
  REQUIRE(shmCliPrinted.empty());
}

TEST_CASE("SharedMemoryCliComponent: show_segments without a value for -s prints an error")
{
  shmCliBackend shmCliServer;
  auto shmCliApi = std::make_shared<SharedMemoryApi>(shmCliProviderOf(shmCliServer.port()));
  SharedMemoryCliComponent shmCliComponent(shmCliApi);

  shmCliCerrCapture shmCliErr;
  shmCliComponent.onCommand(UserDto{}, "show_segments", { "-s" });
  const std::string shmCliReported = shmCliErr.str();

  REQUIRE(shmCliServer.paths.empty());
  REQUIRE(shmCliReported.find("ERROR") != std::string::npos);
}