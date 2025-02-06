#include "data_types.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <string>
#include <tuple>


typedef std::unique_ptr<EVP_CIPHER_CTX, decltype(&::EVP_CIPHER_CTX_free)> EVP_CIPHER_CTX_free_ptr;


static const int KEY_SIZE= 256 / 8;
static const int BLOCK_SIZE = 128 / 8;


void handleOpenSslErrors();
openSslStr aesEncrypt(const EVP_CIPHER* cipher, openSslStr plaintext, unsigned char key[256 / 8]);
openSslStr aesDecrypt(const EVP_CIPHER* cipher, openSslStr ciphertext, unsigned char key[256 / 8]);
