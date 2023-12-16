#include "base_library/core/utils/Cryption.h"

#include <fmt/format.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include <algorithm>

#include "base_library/core/utils/StringUtils.h"

Cryption::Cryption() { ERR_print_errors_fp(stderr); }

std::string Cryption::hashOf(const std::string &text) {
    SHA512_CTX ctx;
    unsigned char buffer[SHA512_DIGEST_LENGTH + 1];

    std::size_t i = 0;
    for (const auto &token: text) {
        buffer[i++] = static_cast<unsigned char>(token);
    }

    SHA512_Init(&ctx);
    SHA512_Update(&ctx, buffer, text.length());
    SHA512_Final(buffer, &ctx);
    buffer[SHA512_DIGEST_LENGTH] = 0;

    std::string tokens;
    for (int j = 0; j < SHA512_DIGEST_LENGTH; j++) {
        unsigned char token = buffer[j];
        tokens += fmt::format("{:02x}", static_cast<int>(token));
    }
    return tokens;
}

std::string Cryption::decodeBase64(const std::string &in) {
    std::string out;
    static const std::string m_lookup =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::vector<int> t(256, -1);
    for (std::size_t i = 0; i < 64; i++) {
        auto iter =
                static_cast<std::size_t>(static_cast<unsigned char>(m_lookup[i]));
        t[iter] = static_cast<int>(i);
    }

    int val = 0;
    int valb = -8;
    for (auto c: in) {
        if (t[static_cast<std::size_t>(c)] == -1) break;
        val = (val << 6) + t[static_cast<std::size_t>(c)];
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

std::string Cryption::encodeBase64(const std::string &in) {
    static const auto m_lookup =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string out;
    out.reserve(in.size());

    int val = 0;
    int valb = -6;

    for (auto c: in) {
        val = (val << 8) + static_cast<uint8_t>(c);
        valb += 8;
        while (valb >= 0) {
            out.push_back(m_lookup[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6) {
        out.push_back(m_lookup[((val << 8) >> (valb + 8)) & 0x3F]);
    }

    while (out.size() % 4) {
        out.push_back('=');
    }

    return out;
}
