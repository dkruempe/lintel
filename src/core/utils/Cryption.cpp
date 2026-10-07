#include "lintel/core/utils/Cryption.h"

#include <fmt/format.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include "lintel/core/utils/StringUtils.h"

namespace {
constexpr std::uint32_t kPbkdf2Iterations = 120000;
constexpr std::size_t kSaltSize = 16;
constexpr std::size_t kDerivedSize = 64;
constexpr const char *kPbkdf2Prefix = "$pbkdf2-sha512$";

void appendHex(std::string &out, const unsigned char *data,
               std::size_t length) {
    for (std::size_t i = 0; i < length; i++) {
        out += fmt::format("{:02x}", static_cast<int>(data[i]));
    }
}

std::string hashWithSalt(const std::string &password,
                         const unsigned char *salt, std::size_t saltSize) {
    unsigned char derived[kDerivedSize];
    if (PKCS5_PBKDF2_HMAC(password.c_str(),
                          static_cast<int>(password.size()), salt,
                          static_cast<int>(saltSize), kPbkdf2Iterations,
                          EVP_sha512(), kDerivedSize, derived) != 1) {
        throw std::runtime_error("Failed to derive key");
    }
    std::string tokens = kPbkdf2Prefix;
    tokens += std::to_string(kPbkdf2Iterations);
    tokens += "$";
    appendHex(tokens, salt, saltSize);
    tokens += "$";
    appendHex(tokens, derived, kDerivedSize);
    return tokens;
}

bool constantTimeEquals(const std::string &a, const std::string &b) {
    if (a.size() != b.size()) {
        return false;
    }
    if (a.empty()) {
        return true;
    }
    return CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}

std::vector<std::string> splitOnDollar(const std::string &value) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= value.size()) {
        std::size_t pos = value.find('$', start);
        if (pos == std::string::npos) {
            parts.push_back(value.substr(start));
            break;
        }
        parts.push_back(value.substr(start, pos - start));
        start = pos + 1;
    }
    return parts;
}

std::vector<unsigned char> hexToBytes(const std::string &hex) {
    std::vector<unsigned char> bytes;
    if (hex.size() % 2 != 0) {
        return bytes;
    }
    bytes.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        unsigned int byte = 0;
        if (std::sscanf(hex.substr(i, 2).c_str(), "%02x", &byte) != 1) {
            return {};
        }
        bytes.push_back(static_cast<unsigned char>(byte));
    }
    return bytes;
}
}  // namespace

Cryption::Cryption() = default;

std::string Cryption::hashOf(const std::string &password) {
    unsigned char salt[kSaltSize];
    if (RAND_bytes(salt, static_cast<int>(kSaltSize)) != 1) {
        throw std::runtime_error("Failed to generate random salt");
    }
    return hashWithSalt(password, salt, kSaltSize);
}

bool Cryption::isModernHash(const std::string &storedHash) {
    return StringUtils::startsWith(storedHash, kPbkdf2Prefix);
}

bool Cryption::verifyOf(const std::string &password,
                        const std::string &storedHash) {
    if (storedHash.empty()) {
        return false;
    }
    if (!isModernHash(storedHash)) {
        // legacy plain SHA-512 hex hash
        return constantTimeEquals(storedHash, hashOfSha512(password));
    }
    const std::vector<std::string> parts = splitOnDollar(storedHash);
    // expected: {"", "pbkdf2-sha512", iterations, salt, derived}
    if (parts.size() != 5 || parts[0] != "" || parts[1] != "pbkdf2-sha512") {
        return false;
    }
    unsigned long iterations = 0;
    try {
        iterations = std::stoul(parts[2]);
    } catch (const std::exception &) {
        return false;
    }
    if (iterations == 0 || iterations > 1000000000UL) {
        return false;
    }
    const std::vector<unsigned char> salt = hexToBytes(parts[3]);
    const std::vector<unsigned char> expected = hexToBytes(parts[4]);
    if (salt.empty() || expected.empty()) {
        return false;
    }
    unsigned char derived[kDerivedSize];
    if (PKCS5_PBKDF2_HMAC(password.c_str(),
                          static_cast<int>(password.size()), salt.data(),
                          static_cast<int>(salt.size()),
                          static_cast<int>(iterations), EVP_sha512(),
                          kDerivedSize, derived) != 1) {
        throw std::runtime_error("Failed to derive key");
    }
    if (expected.size() != kDerivedSize) {
        return false;
    }
    return CRYPTO_memcmp(derived, expected.data(), kDerivedSize) == 0;
}

std::string Cryption::hashOfSha512(const std::string &text) {
    // Create an EVP_MD_CTX context
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    // Initialize the context to use SHA-512
    if (EVP_DigestInit_ex(mdctx, EVP_sha512(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to initialize EVP_MD_CTX with SHA-512");
    }

    // Update the context with the input data
    if (EVP_DigestUpdate(mdctx, text.c_str(), text.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to update EVP_MD_CTX with data");
    }

    // Finalize the hash and get the result
    unsigned char buffer[EVP_MAX_MD_SIZE];
    unsigned int length = 0;
    if (EVP_DigestFinal_ex(mdctx, static_cast<unsigned char *>(buffer), &length) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to finalize EVP_MD_CTX");
    }

    // Clean up the context
    EVP_MD_CTX_free(mdctx);

    // Convert the hash to a hex string
    std::string tokens;
    auto *bufferPtr = static_cast<unsigned char *>(buffer);
    for (unsigned int i = 0; i < length; i++) {
        tokens += fmt::format("{:02x}", static_cast<int>(bufferPtr[i]));
    }

    return tokens;
}


std::string Cryption::decodeBase64(const std::string &in) {
    std::string out;
    static constexpr std::string_view kBase64Lookup =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    static const std::array<int, 256> kReverseTable = [] {
        std::array<int, 256> table{};
        table.fill(-1);
        for (std::size_t i = 0; i < 64; i++) {
            table[static_cast<unsigned char>(kBase64Lookup[i])] = static_cast<int>(i);
        }
        return table;
    }();

    int val = 0;
    int valb = -8;
    for (auto c: in) {
        const std::size_t index =
                static_cast<std::size_t>(static_cast<unsigned char>(c));
        if (kReverseTable[index] == -1) { break; }
        auto uval = static_cast<unsigned>(val);
        uval = (uval << 6U) + static_cast<unsigned>(kReverseTable[index]);
        val = static_cast<int>(uval);
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<char>((static_cast<unsigned>(val) >> static_cast<unsigned>(valb)) & 0xFFU));
            valb -= 8;
        }
    }
    return out;
}

std::string Cryption::encodeBase64(const std::string &in) {
    static constexpr std::string_view kBase64Lookup =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string out;
    out.reserve(in.size());

    int val = 0;
    int valb = -6;

    for (auto c: in) {
        auto uval = static_cast<unsigned>(val);
        uval = (uval << 8U) + static_cast<uint8_t>(c);
        val = static_cast<int>(uval);
        valb += 8;
        while (valb >= 0) {
            out.push_back(kBase64Lookup[(static_cast<unsigned>(val) >> static_cast<unsigned>(valb)) & 0x3FU]);
            valb -= 6;
        }
    }

    if (valb > -6) {
        out.push_back(kBase64Lookup[((static_cast<unsigned>(val) << 8U) >> static_cast<unsigned>(valb + 8)) & 0x3FU]);
    }

    while (out.size() % 4 != 0) {
        out.push_back('=');
    }

    return out;
}
