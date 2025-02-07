#include "util.h"

#include <cmath>
#include <memory>
#include <iostream>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// `ustring`
////////////////////////////////////////////////////////////////////////////////

// no way Google AI wrote code that worked???
// (never mind)
ustring to_ustring(int n) {
    std::string s = std::to_string(n);
    return ustring(s.begin(), s.end());
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

// btw is there a name for that design pattern where yuou pass return val in arg instead like C?
void handleOpenSslErrors() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

ustring prf(const unsigned char* key, int keyLen, ustring input) {
    unsigned char* output = HMAC(EVP_sha512(), key, keyLen, &input[0], input.length(), nullptr, nullptr);
    return ustring(output);
}

ustring aesEncrypt(const EVP_CIPHER* cipher, const unsigned char* key, ustring ptext) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        handleOpenSslErrors();
    }

    // initialize encryption
    if (EVP_EncryptInit_ex(ctx, cipher, nullptr, key, nullptr) != 1) {
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

ustring aesDecrypt(const EVP_CIPHER* cipher, unsigned char* key, ustring ctext) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        handleOpenSslErrors();
    }

    // initialize decryption
    if (EVP_DecryptInit_ex(ctx, cipher, nullptr, key, nullptr) != 1) {
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
    //ptext[ptextLen1 + ptextLen2] = '\0';
    ptext.resize(ptextLen1 + ptextLen2);
    return ptext;
}
