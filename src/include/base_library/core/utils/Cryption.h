#ifndef CPP_BASE_LIBRARY_CRYPTION_H
#define CPP_BASE_LIBRARY_CRYPTION_H

#include <openssl/aes.h>

#include <array>
#include <string>

class Cryption {
private:
    std::array<unsigned char, 32> m_key = {
            117, 56, 56, 84, 104, 053, 111, 067, 077, 050, 050,
            49, 101, 119, 103, 118, 102, 83, 072, 121, 122, 69,
            104, 103, 113, 105, 052, 108, 108, 48, 070, 070};
    std::array<unsigned char, AES_BLOCK_SIZE> m_iv = {
            56, 57, 72, 118, 78, 79, 83, 108, 65, 122, 109, 88, 105, 98, 52, 75};

    static void handleErrors();

public:
    Cryption();

    std::string encryption(const std::string &plainText);

    std::string decryption(const std::string &cipherText);

    static std::string hashOf(const std::string &text);

    static std::string encodeBase64(const std::string &in);

    static std::string decodeBase64(const std::string &in);
};

#endif  // CPP_BASE_LIBRARY_CRYPTION_H
