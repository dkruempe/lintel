#include <httplib.h>

#include <lintel/features/base/configuration/EnvironmentConfiguration.h>
#include <lintel/features/http/service/Client.h>
#include <lintel/features/http/service/ContentType.h>
#include <lintel/features/http/service/HttpStatusCodes.h>
#include <lintel/features/http/service/Server.h>
#include <lintel/features/http/configuration/ServerConfiguration.h>
#include <lintel/features/http/configuration/ClientConfiguration.h>

#include <catch2/catch_all.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

/** How long a test waits for its server thread to start accepting connections. */
constexpr std::chrono::milliseconds kStartupTimeout{ 5000 };

/** Ask the kernel for a free TCP port on 127.0.0.1.
 *
 * `lintel::Server` takes the port in its ServerConfiguration and calls `bind_to_port()`
 * itself, so it cannot use `httplib::bind_to_any_port()` - the port has to be known before the
 * object exists. Binding port 0 lets the kernel choose one and `getsockname()` reads it back. The
 * socket is closed right away, so there is a narrow window in which another process could take the
 * port; if that race is lost, the `Server` constructor throws and the test fails loudly.
 *
 * @return a free port number, or -1 if the socket could not be created or bound */
int freeEphemeralPort()
{
  const int socket = ::socket(AF_INET, SOCK_STREAM, 0);
  if (socket < 0) { return -1; }
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
  address.sin_port = 0;// let the kernel choose
  int port = -1;
  if (::bind(socket, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0) {
    socklen_t length = sizeof(address);
    if (::getsockname(socket, reinterpret_cast<sockaddr *>(&address), &length) == 0) {
      port = ::ntohs(address.sin_port);
    }
  }
  ::close(socket);
  return port;
}

/** Bind a server to a free ephemeral port on 127.0.0.1 and run its accept loop in `serverThread`.
 *
 * `bind_to_any_port()` binds *and* `listen()`s on port 0, so the port is already reserved when this
 * function returns - no other process can take it before the server thread starts. That replaces the
 * fixed ports 18989-18993 these tests used before, which collided with parallel ctest runs and with
 * leftover server processes.
 *
 * @param server server to bind and serve
 * @param serverThread thread that will run the accept loop
 * @return the bound port, or -1 if the socket could not be bound */
int startOnEphemeralPort(httplib::Server &server, std::thread &serverThread)
{
  const int port = server.bind_to_any_port("127.0.0.1");
  if (port <= 0) { return -1; }
  serverThread = std::thread([&server]() { server.listen_after_bind(); });
  return port;
}

/** Wait until the server thread is really accepting connections.
 *
 * @param server server to poll
 * @param timeout maximum time to wait
 * @return true if `is_running()` became true within the timeout */
bool waitUntilRunning(httplib::Server &server, std::chrono::milliseconds timeout)
{
  const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + timeout;
  while (!server.is_running() && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return server.is_running();
}

/** Stops the server and joins its thread on destruction.
 *
 * Needed because a failed REQUIRE aborts the test case by throwing: without this, the joinable
 * std::thread would reach std::terminate in its own destructor and kill the whole test binary. */
struct ServerGuard
{
  httplib::Server &server;
  std::thread &serverThread;

  ~ServerGuard()
  {
    server.stop();
    if (serverThread.joinable()) { serverThread.join(); }
  }
};

}// namespace

TEST_CASE("Server: health and readiness endpoints respond 200 without auth") {
    // Server binds this port itself, so it has to be picked before the object exists. The
    // constructor throws if the bind fails, which fails the test.
    const int port = freeEphemeralPort();
    REQUIRE(port > 0);

    Server server(ServerConfiguration("127.0.0.1", port),
                  std::vector<std::shared_ptr<Controller>>{});

    httplib::Client client("127.0.0.1", port);
    client.set_connection_timeout(std::chrono::seconds(2));
    client.set_read_timeout(std::chrono::seconds(2));

    auto health = client.Get("/health");
    REQUIRE(health != nullptr);
    REQUIRE(health->status == 200);
    REQUIRE(health->body == "{\"status\":\"ok\"}");
    REQUIRE(health->get_header_value("Content-Type") == "application/json");

    auto ready = client.Get("/ready");
    REQUIRE(ready != nullptr);
    REQUIRE(ready->status == 200);
    REQUIRE(ready->body == "{\"status\":\"ready\"}");
    REQUIRE(ready->get_header_value("Content-Type") == "application/json");
}

TEST_CASE("ContentType: parse from string") {
    ContentType ct("application/json");
    REQUIRE(ct.getValue() == ContentType::ApplicationJson);
    REQUIRE(ct.getName() == "application/json");
}

TEST_CASE("ContentType: unknown type returns UNDEFINED") {
    ContentType ct("application/unknown");
    REQUIRE(ct.getValue() == ContentType::UNDEFINED);
}

TEST_CASE("ContentType: text plain") {
    ContentType ct("text/plain");
    REQUIRE(ct.getValue() == ContentType::TextPlain);
    REQUIRE(ct.getName() == "text/plain");
}

TEST_CASE("ContentType: constructor from Value") {
    ContentType ct(ContentType::ApplicationPdf);
    REQUIRE(ct.getName() == "application/pdf");
}

TEST_CASE("HttpStatusCodes: from enum") {
    HttpStatusCodes code(HttpStatusCodes::OK);
    REQUIRE(code.getCode() == 200);
    REQUIRE(code.getValue() == HttpStatusCodes::OK);
}

TEST_CASE("HttpStatusCodes: from int") {
    HttpStatusCodes code(404);
    REQUIRE(code.getCode() == 404);
    REQUIRE(code.getValue() == HttpStatusCodes::NotFound);
}

TEST_CASE("HttpStatusCodes: all codes") {
    REQUIRE(HttpStatusCodes(HttpStatusCodes::BadRequest).getCode() == 400);
    REQUIRE(HttpStatusCodes(HttpStatusCodes::Unauthorized).getCode() == 401);
    REQUIRE(HttpStatusCodes(HttpStatusCodes::InternalServerError).getCode() == 500);
    REQUIRE(HttpStatusCodes(HttpStatusCodes::NoContent).getCode() == 204);
    REQUIRE(HttpStatusCodes(HttpStatusCodes::Created).getCode() == 201);
}

TEST_CASE("ServerConfiguration: default values") {
    ServerConfiguration config("0.0.0.0", 8080);
    REQUIRE(config.getHost() == "0.0.0.0");
    REQUIRE(config.getPort() == 8080);
    REQUIRE(config.getReadTimeOut().count() == 0);
    REQUIRE(config.getWriteTimeOut().count() == 0);
    REQUIRE(config.getIdleInterval().count() == 0);
}

TEST_CASE("ServerConfiguration: with timeouts") {
    ServerConfiguration config("localhost", 9090,
                               std::chrono::milliseconds(3000),
                               std::chrono::milliseconds(5000),
                               std::chrono::milliseconds(60000));
    REQUIRE(config.getReadTimeOut().count() == 3000);
    REQUIRE(config.getWriteTimeOut().count() == 5000);
    REQUIRE(config.getIdleInterval().count() == 60000);
}

TEST_CASE("ServerConfiguration: with cert and key") {
    ServerConfiguration config("0.0.0.0", 443,
                               std::chrono::milliseconds(0),
                               std::chrono::milliseconds(0),
                               std::chrono::milliseconds(0),
                               "/path/to/cert.pem",
                               "/path/to/key.pem");
    REQUIRE(config.getCertFile() == "/path/to/cert.pem");
    REQUIRE(config.getKeyFile() == "/path/to/key.pem");
}

TEST_CASE("ServerConfiguration: with trusted proxies") {
    ServerConfiguration config("0.0.0.0", 8080,
                               std::chrono::milliseconds(0),
                               std::chrono::milliseconds(0),
                               std::chrono::milliseconds(0),
                               "", "", false,
                               {"10.0.0.0/8", "192.168.1.5"});
    auto &proxies = config.getTrustedProxies();
    REQUIRE(proxies.size() == 2);
    REQUIRE(proxies[0] == "10.0.0.0/8");
    REQUIRE(proxies[1] == "192.168.1.5");
}

TEST_CASE("Server: refuses to start without TLS when require_tls is set") {
    ServerConfiguration config("0.0.0.0", 8080,
                               std::chrono::milliseconds(0),
                               std::chrono::milliseconds(0),
                               std::chrono::milliseconds(0),
                               "", "", true);
    REQUIRE_THROWS_AS(Server(config, std::vector<std::shared_ptr<Controller>>{}),
                      std::runtime_error);
}

TEST_CASE("ClientConfiguration: default values") {
    ClientConfiguration config("localhost", 8080,
                               std::chrono::milliseconds(0),
                               std::chrono::milliseconds(0),
                               std::chrono::milliseconds(0));
    REQUIRE(config.getHost() == "localhost");
    REQUIRE(config.getPort() == 8080);
}

TEST_CASE("httplib: server handles GET request") {
    httplib::Server server;
    std::string responseData;
    std::atomic<bool> requestHandled = false;

    server.Get("/hello", [&](const httplib::Request &, httplib::Response &res) {
        responseData = "Hello World!";
        res.set_content("Hello World!", "text/plain");
        requestHandled = true;
    });

    std::thread serverThread;
    const int port = startOnEphemeralPort(server, serverThread);
    ServerGuard guard{ server, serverThread };
    REQUIRE(port > 0);
    REQUIRE(waitUntilRunning(server, kStartupTimeout));

    httplib::Client client("127.0.0.1", port);
    client.set_connection_timeout(std::chrono::seconds(2));
    client.set_read_timeout(std::chrono::seconds(2));

    auto res = client.Get("/hello");
    REQUIRE(res != nullptr);
    REQUIRE(res->status == 200);
    REQUIRE(res->body == "Hello World!");
    REQUIRE(requestHandled);
}

TEST_CASE("httplib: server handles POST with body") {
    httplib::Server server;
    std::string receivedBody;

    server.Post("/echo", [&](const httplib::Request &req, httplib::Response &res) {
        receivedBody = req.body;
        res.set_content(req.body, "text/plain");
    });

    std::thread serverThread;
    const int port = startOnEphemeralPort(server, serverThread);
    ServerGuard guard{ server, serverThread };
    REQUIRE(port > 0);
    REQUIRE(waitUntilRunning(server, kStartupTimeout));

    httplib::Client client("127.0.0.1", port);
    client.set_connection_timeout(std::chrono::seconds(2));
    client.set_read_timeout(std::chrono::seconds(2));

    auto res = client.Post("/echo", "Test Body", "text/plain");
    REQUIRE(res != nullptr);
    REQUIRE(res->status == 200);
    REQUIRE(res->body == "Test Body");
    REQUIRE(receivedBody == "Test Body");
}

TEST_CASE("httplib: server returns 404 for unknown route") {
    httplib::Server server;

    server.Get("/known", [](const httplib::Request &, httplib::Response &res) {
        res.set_content("known", "text/plain");
    });

    std::thread serverThread;
    const int port = startOnEphemeralPort(server, serverThread);
    ServerGuard guard{ server, serverThread };
    REQUIRE(port > 0);
    REQUIRE(waitUntilRunning(server, kStartupTimeout));

    httplib::Client client("127.0.0.1", port);
    client.set_connection_timeout(std::chrono::seconds(2));
    client.set_read_timeout(std::chrono::seconds(2));

    auto res = client.Get("/unknown");
    REQUIRE(res != nullptr);
    REQUIRE(res->status == 404);

    auto known = client.Get("/known");
    REQUIRE(known != nullptr);
    REQUIRE(known->status == 200);
    REQUIRE(known->body == "known");
}

TEST_CASE("Client: uses SSL when TLS is configured") {
    EnvironmentConfiguration envConfig;
    std::filesystem::path configDir =
            envConfig.of(EnvironmentConfiguration::ConfigDirectory);
    std::filesystem::path certPath = configDir / "certs/server.crt";
    std::filesystem::path keyPath = configDir / "certs/server.key";
    REQUIRE(std::filesystem::exists(certPath));
    REQUIRE(std::filesystem::exists(keyPath));

    httplib::SSLServer server(certPath.string().c_str(),
                              keyPath.string().c_str());
    REQUIRE(server.is_valid());
    server.Post("/login", [](const httplib::Request &, httplib::Response &res) {
        res.set_content("{}", "application/json");
    });

    std::thread serverThread;
    const int port = startOnEphemeralPort(server, serverThread);
    ServerGuard guard{ server, serverThread };
    REQUIRE(port > 0);
    REQUIRE(waitUntilRunning(server, kStartupTimeout));

    auto clientConfiguration = std::make_shared<ClientConfiguration>(
            "127.0.0.1", port, std::chrono::milliseconds(0),
            std::chrono::milliseconds(0), std::chrono::seconds(2),
            certPath, keyPath, certPath);
    Client client(clientConfiguration);
    httplib::Result result = client.post("/login", "", "application/json");
    REQUIRE(result != nullptr);
    REQUIRE(result->status == 200);
}
