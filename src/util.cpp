#include "util.h"

#include <cmath>
#include <iostream>
#include <string.h>

int countDigits(int n) {
    if (n == 0) {
        return 1;
    }
    return (int)floor(log10(n)) + 1;
}

int intToUCharPtr(int n, unsigned char* output) {
    std::string s = std::to_string(n);
    int outputLen = s.length();
    memcpy(output, (unsigned char*)(s.c_str()), outputLen);
    return outputLen;
}

// btw is there a name for that design pattern where yuou pass return val in arg instead like C?
void handleOpenSslErrors() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

int prf(unsigned char* key, int keyLen, unsigned char* input, int inputLen, unsigned char* output) {
    unsigned int outputLen;
    HMAC(EVP_sha512(), key, keyLen, input, inputLen, output, &outputLen);
    return (int)outputLen;
}

int aesEncrypt(unsigned char* key, const EVP_CIPHER* cipher, unsigned char* ptext, int ptextLen, unsigned char* ctext) {
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
    if (EVP_EncryptUpdate(ctx, ctext, &ctextLen1, ptext, ptextLen) != 1) {
        handleOpenSslErrors();
    }

    // finalize encryption (deal with last partial block)
    if (EVP_EncryptFinal_ex(ctx, ctext + ctextLen1, &ctextLen2) != 1) {
        handleOpenSslErrors();
    }

    EVP_CIPHER_CTX_free(ctx);
    return ctextLen1 + ctextLen2;
}

int aesDecrypt(unsigned char* key, const EVP_CIPHER* cipher, unsigned char* ctext, int ctextLen, unsigned char* ptext) {
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
    if (EVP_DecryptUpdate(ctx, ptext, &ptextLen1, ctext, ctextLen) != 1) {
        handleOpenSslErrors();
    }

    // finalize decryption (deal with last partial block)
    if (EVP_DecryptFinal_ex(ctx, ptext + ptextLen1, &ptextLen2) != 1) {
        handleOpenSslErrors();
    }

    EVP_CIPHER_CTX_free(ctx);
    //ptext[ptextLen1 + ptextLen2] = '\0';
    return ptextLen1 + ptextLen2;
}
