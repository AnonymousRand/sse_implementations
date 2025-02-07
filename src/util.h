#pragma once

#include <map>
#include <memory>
#include <iostream>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

static const int KEY_SIZE = 256 / 8;
static const int BLOCK_SIZE = 128 / 8;

////////////////////////////////////////////////////////////////////////////////
// `ustring`
////////////////////////////////////////////////////////////////////////////////

// use `ustring` in most of the cases instead of `unsigned char*`
typedef std::basic_string<unsigned char> ustring;

ustring to_ustring(int n);
std::ostream& operator << (std::ostream& os, const ustring str);

////////////////////////////////////////////////////////////////////////////////
// Custom Types
////////////////////////////////////////////////////////////////////////////////

typedef std::tuple<int, int>                    IdRange;
typedef std::tuple<int, int>                    KwRange;
typedef std::tuple<ustring, ustring>            QueryToken;
typedef std::unordered_map<int, int>            Db;
typedef std::map<ustring, std::vector<ustring>> EncIndex;

////////////////////////////////////////////////////////////////////////////////
// OpenSSL
////////////////////////////////////////////////////////////////////////////////

void handleOpenSslErrors();

// PRF implemented as HMAC-SHA512 as done in Private Practical Range Search Revisited
ustring prf(const unsigned char* key, int keyLen, ustring input);

ustring aesEncrypt(const EVP_CIPHER* cipher, const unsigned char* key, ustring ptext);
ustring aesDecrypt(const EVP_CIPHER* cipher, const unsigned char* key, ustring ctext);
