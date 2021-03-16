#include "base_library/configuration/Cryption.h"
#include "base_library/services/LoggerService.h"

#include <openssl/err.h>
#include <openssl/evp.h>

Cryption::Cryption() { ERR_print_errors_fp(stderr); }

std::string Cryption::encryption(const std::string &plainText) {
  // convert plain to unsigned char
  unsigned char plaintext[plainText.length()];
  std::copy(plainText.begin(), plainText.end(), plaintext);
  // init ctx
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  EVP_CIPHER_CTX_init(ctx);
  EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, &key[0], &iv[0]);
  int cipherBlockSize = EVP_CIPHER_CTX_block_size(ctx);
  int cipherKeyLength = EVP_CIPHER_CTX_key_length(ctx);
  int cipherIvLength = EVP_CIPHER_CTX_iv_length(ctx);
  if ((cipherKeyLength > 32) || (cipherIvLength > 16)) {
    LOG_ERROR("Hardwired key or iv was too short!");
  }
  // init out vector
  std::size_t cipherLength = plainText.length() + cipherBlockSize;
  LOG_INFO("cipherBlockSize: {} cipherKeyLength: {} cipherIvLength: {} "
           "cipherLength: {}",
           cipherBlockSize, cipherKeyLength, cipherIvLength, cipherLength);
  unsigned char ciphertext[cipherLength];
  int outBytes;
  if (!EVP_EncryptUpdate(ctx, ciphertext, &outBytes, plaintext,
                         plainText.length())) {
    LOG_ERROR("EVP_EncryptUpdate Failed");
  }
  if (!EVP_EncryptFinal_ex(ctx, ciphertext + outBytes, &outBytes)) {
    LOG_ERROR("EVP_EncryptFinal_ex Failed");
  }
  EVP_CIPHER_CTX_cleanup(ctx);
  // convert out to string
  std::string encrypted;
  for (int i = 0; i < outBytes; i++) {
    encrypted += fmt::format("{0:02X} ", ciphertext[i]);
  }
  return encrypted;
}

std::string Cryption::decryption(const std::string &cipherText) {
  // init ctx
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  EVP_CIPHER_CTX_init(ctx);
  EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, &key[0], &iv[0]);
  int cipherBlockSize = EVP_CIPHER_CTX_block_size(ctx);
  int cipherKeyLength = EVP_CIPHER_CTX_key_length(ctx);
  int cipherIvLength = EVP_CIPHER_CTX_iv_length(ctx);
  if ((cipherKeyLength > 32) || (cipherIvLength > 16)) {
    LOG_ERROR("Hardwired key or iv was too short!");
  }

  int outBytes;
  std::string temp = cipherText;
  std::vector<std::string> vector;
  while (!temp.empty() && temp.size() > 1) {
    auto found = temp.find(' ');
    vector.push_back(temp.substr(0, found));
    temp = temp.substr(found + 1, std::string::npos);
  }
  std::size_t cipherLength = vector.size() + cipherBlockSize - 1;
  unsigned char plaintext[cipherLength];
  memset(plaintext, 0, cipherLength);
  LOG_INFO("cipherBlockSize: {} cipherKeyLength: {} cipherIvLength: {} "
           "cipherLength: {} vector: {}",
           cipherBlockSize, cipherKeyLength, cipherIvLength, cipherLength,
           vector.size());
  unsigned char ciphertext[vector.size()];
  int i = 0;
  for (auto &iter : vector) {
    ciphertext[i] = std::stoul(iter, nullptr, 16);
    i++;
  }
  if (!EVP_DecryptUpdate(ctx, plaintext, &outBytes, ciphertext,
                         vector.size())) {
    LOG_ERROR("EVP_DecryptUpdate Failed");
  }
  if ((cipherLength - (cipherBlockSize + outBytes) <= 0)) {
    LOG_ERROR("Buffer was not big enough to hold decrypted data!");
  }
  if (!EVP_DecryptFinal_ex(ctx, plaintext + outBytes, &outBytes)) {
    ERR_print_errors_fp(stderr);
    LOG_ERROR("EVP_DecryptFinal_ex Failed");
  }
  EVP_CIPHER_CTX_cleanup(ctx);
  std::string plainText;
  for (int i = 0; i < outBytes; i++) {
    plainText += static_cast<char>(plaintext[i]);
  }
  return plainText;
}
