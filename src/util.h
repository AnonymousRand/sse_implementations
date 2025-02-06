#include "data_types.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/params.h>
#include <openssl/core_names.h>
#include <string>
#include <tuple>


typedef std::unique_ptr<EVP_CIPHER_CTX, decltype(&::EVP_CIPHER_CTX_free)> EVP_CIPHER_CTX_free_ptr;


static const int KEY_SIZE= 256 / 8;
static const int BLOCK_SIZE = 128 / 8;


void handleOpenSslErrors();
unsigned char* strToUCharPtr(std::string s);

// PRF implemented as HMAC-SHA512 as done in Private Practical Range Search Revisited
unsigned char* prf(unsigned char* key, int keyLen, unsigned char* input, int inputLen);

int aesEncrypt(const EVP_CIPHER* cipher, unsigned char* ptext, int ptextLen, unsigned char* ctext, unsigned char key[256 / 8]);
int aesDecrypt(const EVP_CIPHER* cipher, unsigned char* ctext, int ctextLen, unsigned char* ptext, unsigned char key[256 / 8]);
