#include <cmath>
#include <memory>
#include <iostream>
#include <string.h>

#include "util.h"

////////////////////////////////////////////////////////////////////////////////
// Custom Types
////////////////////////////////////////////////////////////////////////////////

// todo does general basic_stringstream really not work? would be poggers if
// could use it to generalize int, kwRange, str, ucharptr to ustring
ustring intToUStr(int n) {
    std::string str = std::to_string(n);
    return ustring(str.begin(), str.end());
}

ustring kwRangeToUstr(KwRange kwRange) {
    return intToUStr(kwRange.first) + strToUstr("-") + intToUStr(kwRange.second);
}

ustring strToUstr(std::string s) {
    return reinterpret_cast<const unsigned char*>(s.c_str());
}

ustring ucharptrToUstr(unsigned char* p, size_t len) {
    return ustring(p, len);
}

int ustrToInt(ustring s) {
    std::string str = std::string(s.begin(), s.end());
    return std::stoi(str);
}

bool operator < (const KwRange kwRange1, const KwRange kwRange2) {
    return kwRange1.first < kwRange2.first;
}

std::ostream& operator << (std::ostream& os, const KwRange kwRange) {
    os << kwRange.first << "-" << kwRange.second;
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

// todo btw is there a name for that design pattern where yuou pass return val in arg instead like C?
void handleOpenSslErrors() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

ustring prf(ustring key, ustring input) {
    unsigned int outputLen;
    unsigned char* output = HMAC(EVP_sha512(), &key[0], key.length(), &input[0], input.length(), nullptr, &outputLen);
    return ucharptrToUstr(output, outputLen);
}

ustring genIv() {
    unsigned char* iv = new unsigned char[IV_SIZE];
    int res = RAND_bytes(iv, IV_SIZE);
    if (res != 1) {
        handleOpenSslErrors();
    }
    ustring ustrIv = ucharptrToUstr(iv, IV_SIZE);
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
