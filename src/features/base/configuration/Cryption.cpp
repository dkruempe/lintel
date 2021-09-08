#include "base_library/features/base/configuration/Cryption.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "base_library/core/utils/StringUtils.h"

Cryption::Cryption() { ERR_print_errors_fp(stderr); }

void Cryption::handleErrors() {
  ERR_print_errors_fp(stderr);
  abort();
}

std::string Cryption::encryption(const std::string &plainText) {
  EVP_CIPHER_CTX *ctx;
  int len;
  int ciphertext_len;
  int plaintext_len = static_cast<int>(plainText.length());
  std::vector<unsigned char> ciphertext(plainText.length() * 2);

  /* Create and initialise the context */
  if (!(ctx = EVP_CIPHER_CTX_new())) {
    handleErrors();
  }

  /*
   * Initialise the encryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits
   */
  if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, &m_key[0],
                          &m_iv[0])) {
    handleErrors();
  }

  /*
   * Provide the message to be encrypted, and obtain the encrypted output.
   * EVP_EncryptUpdate can be called multiple times if necessary
   */
  if (!EVP_EncryptUpdate(
          ctx, &ciphertext[0], &len,
          reinterpret_cast<const unsigned char *>(plainText.c_str()),
          plaintext_len)) {
    handleErrors();
  }
  ciphertext_len = len;

  /*
   * Finalise the encryption. Further ciphertext bytes may be written at
   * this stage.
   */
  if (!EVP_EncryptFinal_ex(ctx, &ciphertext[0] + len, &len)) {
    handleErrors();
  }
  ciphertext_len += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  std::string result;
  for (std::size_t i = 0; i < static_cast<std::size_t>(ciphertext_len); i++) {
    result += std::to_string(ciphertext[i]) += " ";
  }
  return result;
}

std::string Cryption::decryption(const std::string &cipherText) {
  EVP_CIPHER_CTX *ctx;

  int len;
  int plaintext_len;
  std::vector<std::string> tokens = StringUtils::split(cipherText, ' ');
  std::vector<unsigned char> ciphertext(tokens.size());
  int ciphertext_len = static_cast<int>(tokens.size());
  std::vector<unsigned char> plaintext(tokens.size());

  std::size_t i = 0;
  for (auto &token : tokens) {
    ciphertext[i++] = static_cast<unsigned char>(std::stoul(token));
  }

  /* Create and initialise the context */
  if (!(ctx = EVP_CIPHER_CTX_new())) {
    handleErrors();
  }

  /*
   * Initialise the decryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits
   */
  if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, &m_key[0],
                          &m_iv[0])) {
    handleErrors();
  }

  /*
   * Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary.
   */
  if (!EVP_DecryptUpdate(ctx, &plaintext[0], &len, &ciphertext[0],
                         ciphertext_len)) {
    handleErrors();
  }
  plaintext_len = len;

  /*
   * Finalise the decryption. Further plaintext bytes may be written at
   * this stage.
   */
  if (!EVP_DecryptFinal_ex(ctx, &plaintext[0] + len, &len)) {
    handleErrors();
  }
  plaintext_len += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  plaintext[static_cast<std::size_t>(plaintext_len)] = 0;
  return reinterpret_cast<const char *>(&plaintext[0]);
}
std::string Cryption::hashOf(const std::string &text) {
  SHA512_CTX ctx;
  std::vector<unsigned char> buffer(SHA512_DIGEST_LENGTH);

  std::size_t i = 0;
  for (auto &token : text) {
    buffer[i++] = static_cast<unsigned char>(token);
  }

  SHA512_Init(&ctx);
  SHA512_Update(&ctx, &buffer[0], text.length());
  SHA512_Final(&buffer[0], &ctx);
  buffer[buffer.size()] = 0;

  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (auto &token : buffer) {
    ss << static_cast<int>(token);
  }
  return ss.str();
}
std::string Cryption::decodeBase64(const std::string &in) {
  std::string out;
  static const std::string lookup =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  std::vector<int> T(in.size());
  for (std::size_t i = 0; i < 64; i++)
    T[static_cast<std::size_t>(lookup[i])] = static_cast<int>(i);

  int val = 0, valb = -8;
  for (auto c : in) {
    if (T[static_cast<std::size_t>(c)] == -1) break;
    val = (val << 6) + T[static_cast<std::size_t>(c)];
    valb += 6;
    if (valb >= 0) {
      out.push_back(char((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return out;
}
