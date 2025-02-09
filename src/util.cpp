#include <cmath>
#include <memory>
#include <iostream>
#include <sstream>
#include <string.h>

#include "util.h"

////////////////////////////////////////////////////////////////////////////////
// Custom Types
////////////////////////////////////////////////////////////////////////////////

int ustrToInt(ustring s) {
    std::string str = std::string(s.begin(), s.end());
    return std::stoi(str);
}

ustring toUstr(int n) {
    std::string str = std::to_string(n);
    return ustring(str.begin(), str.end());
}

ustring toUstr(KwRange kwRange) {
    return toUstr(kwRange.start) + toUstr("-") + toUstr(kwRange.end);
}

ustring toUstr(std::string s) {
    return reinterpret_cast<const unsigned char*>(s.c_str());
}

ustring toUstr(unsigned char* p, int len) {
    return ustring(p, len);
}

KwRange::KwRange(Kw start, Kw end) {
    this->start = start;
    this->end = end;
}

Kw KwRange::size() {
    return (Kw)abs(this->end - this->start);
}

bool KwRange::contains(KwRange kwRange) {
    return this->start <= kwRange.start && this->end >= kwRange.end;
}

bool KwRange::isDisjoint(KwRange kwRange) {
    return this->end < kwRange.start || this->start > kwRange.end;
}

// implemented the same way as `std::pair` does to ensure that this can reflexively determine equivalence
bool operator < (const KwRange& kwRange1, const KwRange& kwRange2) {
    if (kwRange1.start == kwRange2.start) {
        return kwRange1.end < kwRange2.end;
    }
    return kwRange1.start < kwRange2.start;
}

std::ostream& operator << (std::ostream& os, const KwRange& kwRange) {
    os << kwRange.start << "-" << kwRange.end;
    return os;
}

std::ostream& operator << (std::ostream& os, const ustring str) {
    for (auto c : str) {
        os << static_cast<char>(c);
    }
    return os;
}

////////////////////////////////////////////////////////////////////////////////
// OpenSSL
////////////////////////////////////////////////////////////////////////////////

void handleOpenSslErrors() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

ustring prf(ustring key, ustring input) {
    unsigned int outputLen;
    unsigned char* output = HMAC(EVP_sha512(), &key[0], key.length(), &input[0], input.length(), nullptr, &outputLen);
    return toUstr(output, outputLen);
}

ustring genIv() {
    unsigned char* iv = new unsigned char[IV_SIZE];
    int res = RAND_bytes(iv, IV_SIZE);
    if (res != 1) {
        handleOpenSslErrors();
    }
    ustring ustrIv = toUstr(iv, IV_SIZE);
    delete[] iv;
    return ustrIv;
}

ustring aesEncrypt(const EVP_CIPHER* cipher, ustring key, ustring ptext, ustring iv) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        handleOpenSslErrors();
    }

    // initialize encryption
    unsigned char* ucharIv;
    if (iv == ustring()) {
        ucharIv = nullptr;
    } else {
        ucharIv = &iv[0];
    }
    if (EVP_EncryptInit_ex(ctx, cipher, nullptr, &key[0], ucharIv) != 1) {
        handleOpenSslErrors();
    }

    // perform encryption
    int ctextLen1, ctextLen2;
    ustring ctext;
    ctext.resize(ptext.length() + BLOCK_SIZE);
    if (EVP_EncryptUpdate(ctx, &ctext[0], &ctextLen1, &ptext[0], ptext.length()) != 1) {
        handleOpenSslErrors();
    }

    // finalize encryption (deal with last partial block)
    if (EVP_EncryptFinal_ex(ctx, &ctext[0] + ctextLen1, &ctextLen2) != 1) {
        handleOpenSslErrors();
    }

    EVP_CIPHER_CTX_free(ctx);
    ctext.resize(ctextLen1 + ctextLen2);
    return ctext;
}

ustring aesDecrypt(const EVP_CIPHER* cipher, ustring key, ustring ctext, ustring iv) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        handleOpenSslErrors();
    }

    // initialize decryption
    unsigned char* ucharIv;
    if (iv == ustring()) {
        ucharIv = nullptr;
    } else {
        ucharIv = &iv[0];
    }
    if (EVP_DecryptInit_ex(ctx, cipher, nullptr, &key[0], ucharIv) != 1) {
        handleOpenSslErrors();
    }

    // perform decryption
    int ptextLen1, ptextLen2;
    ustring ptext;
    ptext.resize(ctext.length());
    if (EVP_DecryptUpdate(ctx, &ptext[0], &ptextLen1, &ctext[0], ctext.length()) != 1) {
        handleOpenSslErrors();
    }

    // finalize decryption (deal with last partial block)
    if (EVP_DecryptFinal_ex(ctx, &ptext[0] + ptextLen1, &ptextLen2) != 1) {
        handleOpenSslErrors();
    }

    EVP_CIPHER_CTX_free(ctx);
    ptext.resize(ptextLen1 + ptextLen2);
    return ptext;
}
