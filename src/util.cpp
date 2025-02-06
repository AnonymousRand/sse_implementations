#include "util.h"

#include <iostream>


// btw is there a name for that design pattern where yuou pass return val in arg instead like C?
void handleOpenSslErrors() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}


unsigned char* strToUCharPtr(std::string s) {
    return reinterpret_cast<unsigned char*>(const_cast<char*>(s.c_str()));
}


// todo output doesnt match command line!
unsigned char* prf(unsigned char* key, int keyLen, unsigned char* input, int inputLen) {
    return HMAC(EVP_sha512(), key, keyLen, input, inputLen, nullptr, nullptr);
}


int aesEncrypt(const EVP_CIPHER* cipher, unsigned char* ptext, int ptextLen, unsigned char* ctext, unsigned char key[KEY_SIZE]) {
    EVP_CIPHER_CTX_free_ptr ctx(EVP_CIPHER_CTX_new(), ::EVP_CIPHER_CTX_free);
    if (!ctx) {
        handleOpenSslErrors();
    }

    // initialize encryption
    if (EVP_EncryptInit_ex(ctx.get(), cipher, nullptr, key, nullptr) != 1) {
        handleOpenSslErrors();
    }

    // perform encryption
    int ctextLen1, ctextLen2;
    if (EVP_EncryptUpdate(ctx.get(), ctext, &ctextLen1, ptext, ptextLen) != 1) {
        handleOpenSslErrors();
    }

    // finalize encryption (deal with last partial block)
    if (EVP_EncryptFinal_ex(ctx.get(), ctext + ctextLen1, &ctextLen2) != 1) {
        handleOpenSslErrors();
    }

    return ctextLen1 + ctextLen2;
}


int aesDecrypt(const EVP_CIPHER* cipher, unsigned char* ctext, int ctextLen, unsigned char* ptext, unsigned char key[KEY_SIZE]) {
    EVP_CIPHER_CTX_free_ptr ctx(EVP_CIPHER_CTX_new(), ::EVP_CIPHER_CTX_free);
    if (!ctx) {
        handleOpenSslErrors();
    }

    // initialize decryption
    if (EVP_DecryptInit_ex(ctx.get(), cipher, nullptr, key, nullptr) != 1) {
        handleOpenSslErrors();
    }

    // perform decryption
    int ptextLen1, ptextLen2;
    if (EVP_DecryptUpdate(ctx.get(), ptext, &ptextLen1, ctext, ctextLen) != 1) {
        handleOpenSslErrors();
    }

    // finalize decryption (deal with last partial block)
    if (EVP_DecryptFinal_ex(ctx.get(), ptext + ptextLen1, &ptextLen2) != 1) {
        handleOpenSslErrors();
    }

    ptext[ptextLen1 + ptextLen2] = '\0';
    return ptextLen1 + ptextLen2;
}
