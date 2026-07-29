#include "base_library/core/utils/Cryption.h"

#include <fmt/format.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include <stdexcept>
#include <string>

#include "base_library/core/utils/StringUtils.h"

Cryption::Cryption() { ERR_print_errors_fp(stderr); }


std::string Cryption::hashOf(const std::string &text) {
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
        if (t[static_cast<std::size_t>(c)] == -1) { break; }
        auto uval = static_cast<unsigned>(val);
        uval = (uval << 6U) + static_cast<unsigned>(t[static_cast<std::size_t>(c)]);
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
    static const auto m_lookup =
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
            out.push_back(m_lookup[(static_cast<unsigned>(val) >> static_cast<unsigned>(valb)) & 0x3FU]);
            valb -= 6;
        }
    }

    if (valb > -6) {
        out.push_back(m_lookup[((static_cast<unsigned>(val) << 8U) >> static_cast<unsigned>(valb + 8)) & 0x3FU]);
    }

    while (out.size() % 4 != 0) {
        out.push_back('=');
    }

    return out;
}
