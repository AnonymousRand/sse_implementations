#include "util.h"

#include <iostream>


unsigned char* strToUCharPtr(std::string s) {
    return reinterpret_cast<unsigned char*>(const_cast<char*>(s.c_str()));
}


void handleOpenSslErrors() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}


std::tuple<unsigned char*, int> AesEncrypt(unsigned char* plaintext, int plaintextLen, unsigned char* key, int keyLen) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) handleOpenSslErrors();

    // initialize encryption
    int res;
    switch (keyLen) {
        case 128:
            res = EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), nullptr, key, nullptr);
            break;
        case 192:
            res = EVP_EncryptInit_ex(ctx, EVP_aes_192_ecb(), nullptr, key, nullptr);
            break;
        case 256:
            res = EVP_EncryptInit_ex(ctx, EVP_aes_256_ecb(), nullptr, key, nullptr);
            break;
        default:
            std::cerr << "Error: `keyLen` passed to `util.AesEncrypt()` is not valid :/" << std::endl;
            exit(EXIT_FAILURE);
    }
    if (res != 1) handleOpenSslErrors();

    // perform encryption
    unsigned char ciphertext[plaintextLen + keyLen];
    int ciphertextLenSoFar = 0;
    if (EVP_EncryptUpdate(ctx, ciphertext, &ciphertextLenSoFar, plaintext, plaintextLen) != 1) handleOpenSslErrors();

    // finalize encryption (deal with last partial block)
    if (EVP_EncryptFinal_ex(ctx, ciphertext + ciphertextLenSoFar, &ciphertextLenSoFar) != 1) handleOpenSslErrors();

    // clean up
    EVP_CIPHER_CTX_free(ctx);
    return std::tuple<unsigned char*, int> {ciphertext, ciphertextLenSoFar};
}


unsigned char* AesDecrypt(unsigned char* ciphertext, int ciphertextLen, unsigned char* key, int keyLen) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) handleOpenSslErrors();

    // initialize decryption
    int res;
    switch (keyLen) {
        case 128:
            res = EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), nullptr, key, nullptr);
            break;
        case 192:
            res = EVP_DecryptInit_ex(ctx, EVP_aes_192_ecb(), nullptr, key, nullptr);
            break;
        case 256:
            res = EVP_DecryptInit_ex(ctx, EVP_aes_256_ecb(), nullptr, key, nullptr);
            break;
        default:
            std::cerr << "Error: `keyLen` passed to `util.AesDecrypt()` is not valid :/" << std::endl;
            exit(EXIT_FAILURE);
    }
    if (res != 1) handleOpenSslErrors();

    // perform decryption
    unsigned char plaintext[ciphertextLen + 1];
    int plaintextLenSoFar;
    if (EVP_DecryptUpdate(ctx, plaintext, &plaintextLenSoFar, ciphertext, ciphertextLen) != 1) handleOpenSslErrors();

    // finalize decryption (deal with last partial block)
    if (EVP_DecryptFinal_ex(ctx, plaintext + plaintextLenSoFar, &plaintextLenSoFar) != 1) handleOpenSslErrors();

    // clean up
    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}
