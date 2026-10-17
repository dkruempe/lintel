#include <benchmark/benchmark.h>
#include <httplib.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/features/http/configuration/ServerConfiguration.h"
#include "lintel/features/http/service/Controller.h"
#include "lintel/features/http/service/Server.h"

/**
 * HTTP transport throughput (`ROADMAP.md` P2/P6).
 *
 * A real `lintel::Server` on an ephemeral port is driven by a keep-alive
 * `httplib` client, so one iteration measures the request path - accept, route,
 * handler, response - and not the process start.
 *
 * The TLS cases need a certificate pair: they read `LINTEL_BENCH_TLS_CERT` /
 * `LINTEL_BENCH_TLS_KEY`, default to the development pair in `cfg/certs/`, and
 * report "skipped" when it is missing. The CI benchmark job generates the pair
 * with `./cfg/certs/generate_certs.sh` before it runs.
 *
 * `ROADMAP.md` P2 also asks for persistence throughput; `SqliteBenchmark.cpp`
 * covers prepared statements and transaction batches. A server-side cursor and
 * `Notify` benchmark needs a running PostgreSQL, which the advisory benchmark
 * job does not start.
 */

namespace lintel {
namespace {

  /** @return a port that was free a moment ago (the probe socket is closed again) */
  int freePort()
  {
    boost::asio::io_context context;
    boost::asio::ip::tcp::acceptor acceptor(context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));
    const int port = acceptor.local_endpoint().port();
    acceptor.close();
    return port;
  }

  /** @return the certificate pair to serve with, empty paths when unavailable */
  std::pair<std::string, std::string> tlsPair()
  {
    const char *cert = std::getenv("LINTEL_BENCH_TLS_CERT");
    const char *key = std::getenv("LINTEL_BENCH_TLS_KEY");
    if (cert != nullptr && key != nullptr) { return { cert, key }; }
    EnvironmentConfiguration environmentConfiguration;
    const std::filesystem::path directory = environmentConfiguration.of(EnvironmentConfiguration::ConfigDirectory);
    const std::filesystem::path certificate = directory / "certs/server.crt";
    const std::filesystem::path privateKey = directory / "certs/server.key";
    if (!std::filesystem::exists(certificate) || !std::filesystem::exists(privateKey)) { return {}; }
    return { certificate.string(), privateKey.string() };
  }

  /** A running server plus the port it listens on. */
  struct ServerUnderTest
  {
    std::unique_ptr<Server> m_server;
    int m_port = 0;
  };

  ServerUnderTest startServer(const std::string &certFile, const std::string &keyFile)
  {
    ServerUnderTest result;
    result.m_port = freePort();
    const ServerConfiguration configuration("127.0.0.1",
      result.m_port,
      std::chrono::milliseconds(3000),
      std::chrono::milliseconds(5000),
      std::chrono::milliseconds(60000),
      certFile,
      keyFile);
    result.m_server = std::make_unique<Server>(configuration, std::vector<std::shared_ptr<Controller>>{});
    return result;
  }

  template<class Client> void configureClient(Client &client)
  {
    client.set_tcp_nodelay(true);
    client.set_connection_timeout(std::chrono::seconds(5));
    client.set_read_timeout(std::chrono::seconds(5));
    client.set_write_timeout(std::chrono::seconds(5));
  }

  /** @return true once the server answers `GET /health` with 200 */
  template<class Client> bool waitForServer(benchmark::State &state, Client &client)
  {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
      if (auto response = client.Get("/health"); response != nullptr && response->status == 200) { return true; }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    state.SkipWithMessage("the server did not start listening");
    return false;
  }

  /** One request per iteration over a keep-alive connection. */
  template<class Client>
  void runKeepAlive(benchmark::State &state,
    const std::string &path,
    const std::string &certFile,
    const std::string &keyFile)
  {
    const ServerUnderTest server = startServer(certFile, keyFile);
    Client client("127.0.0.1", server.m_port);
    configureClient(client);
    if (!waitForServer(state, client)) { return; }

    std::size_t bodyBytes = 0;
    for (auto _ : state) {
      auto response = client.Get(path);
      if (response == nullptr || response->status != 200) {
        state.SkipWithMessage("the server did not answer with 200");
        return;
      }
      bodyBytes = response->body.size();
      benchmark::DoNotOptimize(response->body);
    }
    state.counters["body_bytes"] = static_cast<double>(bodyBytes);
  }

  /** One request per new connection: pays the connect (and handshake) path. */
  void runNewConnection(benchmark::State &state,
    const std::string &path,
    const std::string &certFile,
    const std::string &keyFile)
  {
    const ServerUnderTest server = startServer(certFile, keyFile);
    {
      httplib::Client probe("127.0.0.1", server.m_port);
      configureClient(probe);
      if (!waitForServer(state, probe)) { return; }
    }
    std::size_t bodyBytes = 0;
    for (auto _ : state) {
      httplib::Client client("127.0.0.1", server.m_port);
      configureClient(client);
      auto response = client.Get(path);
      if (response == nullptr || response->status != 200) {
        state.SkipWithMessage("the server did not answer with 200");
        return;
      }
      bodyBytes = response->body.size();
      benchmark::DoNotOptimize(response->body);
    }
    state.counters["body_bytes"] = static_cast<double>(bodyBytes);
  }

  /** @return the TLS pair, reporting the case as skipped when there is none */
  bool tlsPairOrSkip(benchmark::State &state, std::pair<std::string, std::string> &pair)
  {
    pair = tlsPair();
    if (pair.first.empty()) {
      state.SkipWithMessage("no certificate pair (run ./cfg/certs/generate_certs.sh)");
      return false;
    }
    return true;
  }

  /** Keep-alive TLS throughput; the certificate is a development one, so trust is not measured. */
  void runKeepAliveTls(benchmark::State &state, const std::string &path)
  {
    std::pair<std::string, std::string> pair;
    if (!tlsPairOrSkip(state, pair)) { return; }
    const ServerUnderTest server = startServer(pair.first, pair.second);
    httplib::SSLClient client("127.0.0.1", server.m_port);
    configureClient(client);
    client.enable_server_certificate_verification(false);
    if (!waitForServer(state, client)) { return; }

    std::size_t bodyBytes = 0;
    for (auto _ : state) {
      auto response = client.Get(path);
      if (response == nullptr || response->status != 200) {
        state.SkipWithMessage("the TLS server did not answer with 200");
        return;
      }
      bodyBytes = response->body.size();
      benchmark::DoNotOptimize(response->body);
    }
    state.counters["body_bytes"] = static_cast<double>(bodyBytes);
  }

}// namespace

static void BM_Http_Health_PlainKeepAlive(benchmark::State &state)
{
  runKeepAlive<httplib::Client>(state, "/health", "", "");
}
BENCHMARK(BM_Http_Health_PlainKeepAlive);

static void BM_Http_Ready_PlainKeepAlive(benchmark::State &state)
{
  runKeepAlive<httplib::Client>(state, "/ready", "", "");
}
BENCHMARK(BM_Http_Ready_PlainKeepAlive);

static void BM_Http_Health_PlainNewConnection(benchmark::State &state) { runNewConnection(state, "/health", "", ""); }
BENCHMARK(BM_Http_Health_PlainNewConnection);

static void BM_Http_Health_TlsKeepAlive(benchmark::State &state) { runKeepAliveTls(state, "/health"); }
BENCHMARK(BM_Http_Health_TlsKeepAlive);

static void BM_Http_Ready_TlsKeepAlive(benchmark::State &state) { runKeepAliveTls(state, "/ready"); }
BENCHMARK(BM_Http_Ready_TlsKeepAlive);

}// namespace lintel
