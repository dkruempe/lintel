#include "base_library/features/http/service/ClientIpResolver.h"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/network_v6.hpp>

#include <array>
#include <algorithm>
#include <string>
#include <utility>

namespace {

std::string trimHeaderValue(const std::string &value) {
    const std::string whitespace = " \t\r\n";
    const std::size_t first = value.find_first_not_of(whitespace);
    if (first == std::string::npos) {
        return {};
    }
    const std::size_t last = value.find_last_not_of(whitespace);
    return value.substr(first, last - first + 1);
}

bool isValidIpAddress(const std::string &value) {
    boost::system::error_code error;
    boost::asio::ip::make_address(value, error);
    return !error;
}

bool matchesCidrV4(const std::string &remoteAddress, const std::string &pattern) {
    boost::system::error_code error;
    const boost::asio::ip::network_v4 network =
            boost::asio::ip::make_network_v4(pattern, error);
    if (error) {
        return false;
    }
    const boost::asio::ip::address_v4 remote =
            boost::asio::ip::make_address_v4(remoteAddress, error);
    if (error) {
        return false;
    }
    const auto remoteUint = remote.to_uint();
    const auto mask = network.netmask().to_uint();
    const auto netAddress = network.address().to_uint();
    return (remoteUint & mask) == (netAddress & mask);
}

bool matchesCidrV6(const std::string &remoteAddress, const std::string &pattern) {
    boost::system::error_code error;
    const boost::asio::ip::network_v6 network =
            boost::asio::ip::make_network_v6(pattern, error);
    if (error) {
        return false;
    }
    const boost::asio::ip::address_v6 remote =
            boost::asio::ip::make_address_v6(remoteAddress, error);
    if (error) {
        return false;
    }
    const unsigned int prefix = network.prefix_length();
    std::array<unsigned char, 16> maskBytes{};
    auto maskIt = maskBytes.begin();
    for (std::size_t bit = 0; maskIt != maskBytes.end(); ++maskIt, bit += 8) {
        const auto firstBit = static_cast<unsigned int>(bit);
        if (prefix >= firstBit + 8) {
            *maskIt = 0xFF;
        } else if (prefix > firstBit) {
            *maskIt = static_cast<unsigned char>(0xFFU << (8 - (prefix - firstBit)));
        }
    }
    const std::array<unsigned char, 16> remoteBytes = remote.to_bytes();
    const std::array<unsigned char, 16> canonicalBytes =
            network.canonical().address().to_bytes();
    auto remoteIt = remoteBytes.begin();
    auto maskByte = maskBytes.begin();
    auto canonicalIt = canonicalBytes.begin();
    for (; remoteIt != remoteBytes.end();
         ++remoteIt, ++maskByte, ++canonicalIt) {
        if ((*remoteIt & *maskByte) != *canonicalIt) {
            return false;
        }
    }
    return true;
}

/** @return true if remoteAddress matches an exact IP or CIDR pattern. */
bool matchesIpPattern(const std::string &remoteAddress,
                      const std::string &pattern) {
    boost::system::error_code error;
    const boost::asio::ip::address remote =
            boost::asio::ip::make_address(remoteAddress, error);
    if (error) {
        return false;
    }
    const std::size_t slash = pattern.find('/');
    if (slash == std::string::npos) {
        const boost::asio::ip::address expected =
                boost::asio::ip::make_address(pattern, error);
        return !error && expected == remote;
    }
    if (remote.is_v4()) {
        return matchesCidrV4(remoteAddress, pattern);
    }
    return matchesCidrV6(remoteAddress, pattern);
}

/** @return the leftmost, trimmed entry of an X-Forwarded-For header. */
std::string firstForwardedIp(const std::string &forwardedFor) {
    if (forwardedFor.empty()) {
        return {};
    }
    const std::size_t comma = forwardedFor.find(',');
    return trimHeaderValue(forwardedFor.substr(0, comma));
}

}  // namespace

ClientIpResolver::ClientIpResolver(std::vector<std::string> trustedProxies)
    : m_trustedProxies(std::move(trustedProxies)) {}

void ClientIpResolver::setTrustedProxies(
        std::vector<std::string> trustedProxies) {
    m_trustedProxies = std::move(trustedProxies);
}

std::string ClientIpResolver::resolveClientIp(
        const std::string &remoteAddress, const std::string &forwardedFor) const {
    if (remoteAddress.empty()) {
        return {};
    }
    if (!isTrustedPeer(remoteAddress)) {
        return remoteAddress;
    }
    std::string forwarded = firstForwardedIp(forwardedFor);
    if (forwarded.empty() || !isValidIpAddress(forwarded)) {
        return remoteAddress;
    }
    return forwarded;
}

bool ClientIpResolver::isTrustedPeer(const std::string &remoteAddress) const {
    if (m_trustedProxies.empty()) {
        return false;
    }
    return std::any_of(m_trustedProxies.begin(), m_trustedProxies.end(),
                       [&remoteAddress](const std::string &pattern) {
                           return matchesIpPattern(remoteAddress, pattern);
                       });
}
