#ifndef CPP_BASE_LIBRARY_CRYPTION_H
#define CPP_BASE_LIBRARY_CRYPTION_H

#include <openssl/aes.h>

#include <array>
#include <string>

class Cryption {
public:
    Cryption();

    static std::string hashOf(const std::string &text);

    static std::string encodeBase64(const std::string &in);

    static std::string decodeBase64(const std::string &in);
};

#endif  // CPP_BASE_LIBRARY_CRYPTION_H
