#pragma once

#include <map>
#include <iostream>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
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
ustring prf(ustring key, ustring input);

ustring aesEncrypt(const EVP_CIPHER* cipher, ustring key, ustring ptext, ustring iv=ustring());
ustring aesDecrypt(const EVP_CIPHER* cipher, ustring key, ustring ctext, ustring iv=ustring());
