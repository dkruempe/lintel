#include "base_library/configuration/Cryption.h"
#include "base_library/services/LoggerService.h"

#include <algorithm>
#include <cstdio>
#include <openssl/err.h>
#include <openssl/evp.h>

Cryption::Cryption() { ERR_print_errors_fp(stderr); }

void Cryption::handleErrors(void) {
  ERR_print_errors_fp(stderr);
  abort();
}

std::string Cryption::encryption(const std::string &plainText) {
  EVP_CIPHER_CTX *ctx;
  int len;
  int ciphertext_len;
  int plaintext_len = plainText.length();
  unsigned char ciphertext[plainText.length() * 2];

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
  if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, &key[0], &iv[0])) {
    handleErrors();
  }

  /*
   * Provide the message to be encrypted, and obtain the encrypted output.
   * EVP_EncryptUpdate can be called multiple times if necessary
   */
  if (!EVP_EncryptUpdate(ctx, ciphertext, &len,
                         (unsigned char *)(plainText.c_str()), plaintext_len)) {
    handleErrors();
  }
  ciphertext_len = len;

  /*
   * Finalise the encryption. Further ciphertext bytes may be written at
   * this stage.
   */
  if (!EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
    handleErrors();
  }
  ciphertext_len += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  printf("Ciphertext is:\n");
  BIO_dump_fp(stdout, (const char *)ciphertext, ciphertext_len);

  std::string result;
  for (int i = 0; i < ciphertext_len; i++) {
    result += static_cast<char>(ciphertext[i]);
  }
  return result;
}

std::string Cryption::decryption(const std::string &cipherText) {
  EVP_CIPHER_CTX *ctx;

  int len;
  int plaintext_len;
  unsigned char ciphertext[cipherText.length()];
  int ciphertext_len = cipherText.length();
  unsigned char plaintext[cipherText.length()];

  std::copy(cipherText.begin(), cipherText.end(), ciphertext);

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
  if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, &key[0], &iv[0])) {
    handleErrors();
  }

  /*
   * Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary.
   */
  if (!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
    handleErrors();
  }
  plaintext_len = len;

  /*
   * Finalise the decryption. Further plaintext bytes may be written at
   * this stage.
   */
  if (!EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
    handleErrors();
  }
  plaintext_len += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  plaintext[plaintext_len] = 0;
  return reinterpret_cast<char *>(plaintext);
}
