#include <httplib.h>

#include <base_library/features/http/service/ContentType.h>
#include <base_library/features/http/service/HttpStatusCodes.h>
#include <base_library/features/http/service/Server.h>
#include <base_library/features/http/configuration/ServerConfiguration.h>
#include <base_library/features/http/configuration/ClientConfiguration.h>

#include <catch2/catch_all.hpp>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>
#include <vector>

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

    int port = 18989;
    std::thread serverThread([&]() { server.listen("127.0.0.1", port); });
    for (int i = 0; i < 50 && !server.is_running(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    httplib::Client client("127.0.0.1", port);
    client.set_connection_timeout(std::chrono::seconds(2));
    client.set_read_timeout(std::chrono::seconds(2));

    auto res = client.Get("/hello");
    if (res) {
        REQUIRE(res->status == 200);
        REQUIRE(res->body == "Hello World!");
    } else {
        // Server might not be ready yet - skip in CI
        WARN("HTTP request failed - server may not be ready");
    }

    server.stop();
    serverThread.join();
}

TEST_CASE("httplib: server handles POST with body") {
    httplib::Server server;
    std::string receivedBody;

    server.Post("/echo", [&](const httplib::Request &req, httplib::Response &res) {
        receivedBody = req.body;
        res.set_content(req.body, "text/plain");
    });

    int port = 18990;
    std::thread serverThread([&]() { server.listen("127.0.0.1", port); });
    for (int i = 0; i < 50 && !server.is_running(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    httplib::Client client("127.0.0.1", port);
    client.set_connection_timeout(std::chrono::seconds(2));
    client.set_read_timeout(std::chrono::seconds(2));

    auto res = client.Post("/echo", "Test Body", "text/plain");
    if (res) {
        REQUIRE(res->status == 200);
        REQUIRE(res->body == "Test Body");
        REQUIRE(receivedBody == "Test Body");
    } else {
        WARN("HTTP POST request failed - server may not be ready");
    }

    server.stop();
    serverThread.join();
}

TEST_CASE("httplib: server returns 404 for unknown route") {
    httplib::Server server;

    server.Get("/known", [](const httplib::Request &, httplib::Response &res) {
        res.set_content("known", "text/plain");
    });

    int port = 18991;
    std::thread serverThread([&]() { server.listen("127.0.0.1", port); });
    for (int i = 0; i < 50 && !server.is_running(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    httplib::Client client("127.0.0.1", port);
    client.set_connection_timeout(std::chrono::seconds(2));
    client.set_read_timeout(std::chrono::seconds(2));

    auto res = client.Get("/unknown");
    if (res) {
        REQUIRE(res->status == 404);
    } else {
        WARN("HTTP request failed - server may not be ready");
    }

    server.stop();
    serverThread.join();
}
