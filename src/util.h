#pragma once

#include <map>
#include <memory>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

typedef std::tuple<int, int>                                  IdRange;
typedef std::tuple<int, int>                                  KwRange;
typedef std::tuple<unsigned char*, unsigned char*>            QueryToken;
typedef std::unordered_map<int, int>                          Db;
typedef std::map<unsigned char*, std::vector<unsigned char*>> EncIndex;
typedef std::basic_string<unsigned char>                      ustring;

static const int KEY_SIZE = 256 / 8;
static const int BLOCK_SIZE = 128 / 8;

// assumes numbers are positive
int countDigits(int n);
int intToUCharPtr(int n, unsigned char* output);

void handleOpenSslErrors();

// PRF implemented as HMAC-SHA512 as done in Private Practical Range Search Revisited
int prf(unsigned char* key, int keyLen, unsigned char* input, int inputLen, unsigned char* output);

int aesEncrypt(unsigned char* key, const EVP_CIPHER* cipher, unsigned char* ptext, int ptextLen, unsigned char* ctext);
int aesDecrypt(unsigned char* key, const EVP_CIPHER* cipher, unsigned char* ctext, int ctextLen, unsigned char* ptext);
