#include "lintel/features/http/service/ClientIpResolver.h"

#include <catch2/catch_all.hpp>

TEST_CASE("ClientIpResolver: ignores X-Forwarded-For from untrusted peer") {
    ClientIpResolver resolver;
    REQUIRE(resolver.resolveClientIp("10.0.0.1", "203.0.113.7") == "10.0.0.1");
}

TEST_CASE("ClientIpResolver: ignores X-Forwarded-For when no proxies configured") {
    ClientIpResolver resolver(std::vector<std::string>{});
    REQUIRE(resolver.resolveClientIp("127.0.0.1", "203.0.113.7") == "127.0.0.1");
}

TEST_CASE("ClientIpResolver: honors X-Forwarded-For from trusted exact IP") {
    ClientIpResolver resolver({"10.0.0.1"});
    REQUIRE(resolver.resolveClientIp("10.0.0.1", "203.0.113.7") == "203.0.113.7");
}

TEST_CASE("ClientIpResolver: honors X-Forwarded-For from trusted CIDR") {
    ClientIpResolver resolver({"10.0.0.0/8"});
    REQUIRE(resolver.resolveClientIp("10.11.12.13", "203.0.113.7") == "203.0.113.7");
}

TEST_CASE("ClientIpResolver: ignores X-Forwarded-For from peer outside CIDR") {
    ClientIpResolver resolver({"10.0.0.0/8"});
    REQUIRE(resolver.resolveClientIp("192.168.1.5", "203.0.113.7") == "192.168.1.5");
}

TEST_CASE("ClientIpResolver: uses leftmost entry of X-Forwarded-For") {
    ClientIpResolver resolver({"10.0.0.0/8"});
    REQUIRE(resolver.resolveClientIp("10.0.0.9", "203.0.113.7, 10.0.0.1") ==
            "203.0.113.7");
}

TEST_CASE("ClientIpResolver: trims whitespace in X-Forwarded-For") {
    ClientIpResolver resolver({"10.0.0.0/8"});
    REQUIRE(resolver.resolveClientIp("10.0.0.9", "  203.0.113.7  ") ==
            "203.0.113.7");
}

TEST_CASE("ClientIpResolver: falls back to remote address on empty X-Forwarded-For") {
    ClientIpResolver resolver({"10.0.0.0/8"});
    REQUIRE(resolver.resolveClientIp("10.0.0.9", "") == "10.0.0.9");
}

TEST_CASE("ClientIpResolver: falls back to remote address on invalid forwarded IP") {
    ClientIpResolver resolver({"10.0.0.0/8"});
    REQUIRE(resolver.resolveClientIp("10.0.0.9", "not-an-ip") == "10.0.0.9");
}

TEST_CASE("ClientIpResolver: IPv6 trusted peer via CIDR") {
    ClientIpResolver resolver({"2001:db8::/32"});
    REQUIRE(resolver.resolveClientIp("2001:db8::1", "203.0.113.7") == "203.0.113.7");
}

TEST_CASE("ClientIpResolver: IPv6 X-Forwarded-For from trusted peer") {
    ClientIpResolver resolver({"10.0.0.0/8"});
    REQUIRE(resolver.resolveClientIp("10.0.0.9", "2001:db8::5") == "2001:db8::5");
}

TEST_CASE("ClientIpResolver: setTrustedProxies updates configuration") {
    ClientIpResolver resolver;
    resolver.setTrustedProxies({"10.0.0.0/8"});
    REQUIRE(resolver.resolveClientIp("10.0.0.9", "203.0.113.7") == "203.0.113.7");
}
