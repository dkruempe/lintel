#ifndef CPP_BASE_LIBRARY_CRYPTION_H
#define CPP_BASE_LIBRARY_CRYPTION_H

#include <array>
#include <openssl/aes.h>
#include <string>

class Cryption {
private:
  std::array<unsigned char, 32> key = {117, 56,  56,  84,  104, 053, 111, 067,
                                       077, 050, 050, 49,  101, 119, 103, 118,
                                       102, 83,  072, 121, 122, 69,  104, 103,
                                       113, 105, 052, 108, 108, 48,  070, 070};
  std::array<unsigned char, AES_BLOCK_SIZE> iv = {
      56, 57, 72, 118, 78, 79, 83, 108, 65, 122, 109, 88, 105, 98, 52, 75};

  static void handleErrors();

public:
  Cryption();

  std::string encryption(const std::string &plainText);

  std::string decryption(const std::string &cipherText);
};

#endif // CPP_BASE_LIBRARY_CRYPTION_H
