#include "util.h"

#include <iostream>


// btw is there a name for that design pattern where yuou pass return val in arg instead like C?
void handleOpenSslErrors() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}


// todo need int to OpenSslStr
// todo implement prf with hmac-sha-512 as in paper?
openSslStr intToOpenSslStr(int n) {
    openSslStr s;
    s.c_str() = std::to_string(n);
}


openSslStr aesEncrypt(const EVP_CIPHER* cipher, openSslStr plaintext, unsigned char key[KEY_SIZE]) {
    EVP_CIPHER_CTX_free_ptr ctx(EVP_CIPHER_CTX_new(), ::EVP_CIPHER_CTX_free);
    if (!ctx) {
        handleOpenSslErrors();
    }

    // initialize encryption
    if (EVP_EncryptInit_ex(ctx.get(), cipher, nullptr, key, nullptr) != 1) {
        handleOpenSslErrors();
    }

    // perform encryption
    openSslStr ciphertext;
    ciphertext.resize(plaintext.size() + BLOCK_SIZE);
    int ciphertextLen1, ciphertextLen2;
    if (EVP_EncryptUpdate(ctx.get(), (unsigned char*)&ciphertext[0], &ciphertextLen1, (unsigned char*)&plaintext[0], plaintext.size()) != 1) {
        handleOpenSslErrors();
    }

    // finalize encryption (deal with last partial block)
    if (EVP_EncryptFinal_ex(ctx.get(), (unsigned char*)&ciphertext[0] + ciphertextLen1, &ciphertextLen2) != 1) {
        handleOpenSslErrors();
    }

    ciphertext.resize(ciphertextLen1 + ciphertextLen2);
    return ciphertext;
}


openSslStr aesDecrypt(const EVP_CIPHER* cipher, openSslStr ciphertext, unsigned char key[KEY_SIZE]) {
    EVP_CIPHER_CTX_free_ptr ctx(EVP_CIPHER_CTX_new(), ::EVP_CIPHER_CTX_free);
    if (!ctx) {
        handleOpenSslErrors();
    }

    // initialize decryption
    if (EVP_DecryptInit_ex(ctx.get(), cipher, nullptr, key, nullptr) != 1) {
        handleOpenSslErrors();
    }

    // perform decryption
    openSslStr plaintext;
    plaintext.resize(ciphertext.size());
    int plaintextLen1, plaintextLen2;
    if (EVP_DecryptUpdate(ctx.get(), (unsigned char*)&plaintext[0], &plaintextLen1, (unsigned char*)&ciphertext[0], ciphertext.size()) != 1) {
        handleOpenSslErrors();
    }

    // finalize decryption (deal with last partial block)
    if (EVP_DecryptFinal_ex(ctx.get(), (unsigned char*)&plaintext[0] + plaintextLen1, &plaintextLen2) != 1) {
        handleOpenSslErrors();
    }

    plaintext.resize(plaintextLen1 + plaintextLen2);
    return plaintext;
}
