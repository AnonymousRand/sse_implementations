#include "util.h"

#include <iostream>


unsigned char* strToUCharPtr(std::string s) {
    return reinterpret_cast<unsigned char*>(const_cast<char*>(s.c_str()));
}


void handleOpenSslErrors() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}


std::tuple<unsigned char*, int> aesEncrypt(const EVP_CIPHER* cipher, unsigned char* plaintext, int plaintextLen, unsigned char* key, int blockSize) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) handleOpenSslErrors();

    // initialize encryption
    if (EVP_EncryptInit_ex(ctx, cipher, nullptr, key, nullptr) != 1) handleOpenSslErrors();

    // perform encryption
    unsigned char ciphertext[plaintextLen + blockSize];
    int ciphertextLenSoFar = 0;
    if (EVP_EncryptUpdate(ctx, ciphertext, &ciphertextLenSoFar, plaintext, plaintextLen) != 1) handleOpenSslErrors();

    // finalize encryption (deal with last partial block)
    if (EVP_EncryptFinal_ex(ctx, ciphertext + ciphertextLenSoFar, &ciphertextLenSoFar) != 1) handleOpenSslErrors();

    // clean up
    EVP_CIPHER_CTX_free(ctx);
    // todo also for int to uchar* just do C-style of passing return value in argument instead?
    // btw is there a name for that design pattern?
    return std::tuple<unsigned char*, int> {ciphertext, ciphertextLenSoFar};
}


unsigned char* aesDecrypt(const EVP_CIPHER* cipher, unsigned char* ciphertext, int ciphertextLen, unsigned char* key) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) handleOpenSslErrors();

    // initialize decryption
    if (EVP_DecryptInit_ex(ctx, cipher, nullptr, key, nullptr) != 1) handleOpenSslErrors();

    // perform decryption
    unsigned char plaintext[ciphertextLen + 1];
    int plaintextLenSoFar;
    if (EVP_DecryptUpdate(ctx, plaintext, &plaintextLenSoFar, ciphertext, ciphertextLen) != 1) handleOpenSslErrors();

    // finalize decryption (deal with last partial block)
    if (EVP_DecryptFinal_ex(ctx, plaintext + plaintextLenSoFar, &plaintextLenSoFar) != 1) handleOpenSslErrors();
    plaintext[plaintextLenSoFar] = '\0'; // need to add back null terminator

    // clean up
    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}
