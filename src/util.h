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

class KwRange {
    public:
        Kw start;
        Kw end;

        KwRange() = default;
        KwRange(Kw start, Kw end);
        Kw size();
        bool contains(KwRange kwRange);
        bool isDisjoint(KwRange kwRange);

        friend bool operator < (const KwRange& kwRange1, const KwRange& kwRange2);
        friend std::ostream& operator << (std::ostream& os, const KwRange& kwRange);
};

typedef std::pair<ustring, ustring>                    QueryToken;
// `multimap` to allow documents to contain multiple keywords (keyword search problem)
// as well as work with schemes that replicate documents/tuples
typedef std::unordered_multimap<Id, KwRange>           Db;
// `std::map<label, std::pair<data, iv>>`
typedef std::map<ustring, std::pair<ustring, ustring>> EncIndex;

int ustrToInt(ustring n);
ustring toUstr(int n);
ustring toUstr(KwRange kwRange);
ustring toUstr(std::string s);
ustring toUstr(unsigned char* p, int len);

std::ostream& operator << (std::ostream& os, const ustring str);

////////////////////////////////////////////////////////////////////////////////
// OpenSSL
////////////////////////////////////////////////////////////////////////////////

void handleOpenSslErrors();

// PRF implemented as HMAC-SHA512 as done in Private Practical Range Search Revisited
ustring prf(ustring key, ustring input);

ustring genIv();
ustring aesEncrypt(const EVP_CIPHER* cipher, ustring key, ustring ptext, ustring iv = ustring());
ustring aesDecrypt(const EVP_CIPHER* cipher, ustring key, ustring ctext, ustring iv = ustring());
