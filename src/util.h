#pragma once

#include <map>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

static const int KEY_SIZE = 256 / 8;
static const int BLOCK_SIZE = 128 / 8;
static const int IV_SIZE = 128 / 8;


////////////////////////////////////////////////////////////////////////////////
// Custom Types
////////////////////////////////////////////////////////////////////////////////

// use `ustring` as much as possible instead of `unsigned char*` to avoid C-style hell
typedef std::basic_string<unsigned char> ustring;

typedef int                                            Id;
typedef int                                            Kw;
typedef std::pair<Id, Id>                              IdRange;
// `std::pair<kw_range_start, kw_range_end>` to be generalized for ranges
typedef std::pair<Kw, Kw>                              KwRange;
typedef std::pair<ustring, ustring>                    QueryToken;
typedef std::unordered_map<Id, KwRange>                Db;
// `std::map<label, std::pair<data, iv>>`
typedef std::map<ustring, std::pair<ustring, ustring>> EncIndex;

ustring intToUStr(int n);
ustring kwRangeToUstr(KwRange kwRange);
ustring strToUstr(std::string s);
ustring ucharptrToUstr(unsigned char* p, size_t len);
int ustrToInt(ustring n);

bool operator < (const KwRange kwRange1, const KwRange kwRange2);
std::ostream& operator << (std::ostream& os, const KwRange kwRange);
std::ostream& operator << (std::ostream& os, const ustring str);

////////////////////////////////////////////////////////////////////////////////
// OpenSSL
////////////////////////////////////////////////////////////////////////////////

void handleOpenSslErrors();

// PRF implemented as HMAC-SHA512 as done in Private Practical Range Search Revisited
ustring prf(ustring key, ustring input);

ustring genIv();
ustring aesEncrypt(const EVP_CIPHER* cipher, ustring key, ustring ptext, ustring iv=ustring());
ustring aesDecrypt(const EVP_CIPHER* cipher, ustring key, ustring ctext, ustring iv=ustring());
