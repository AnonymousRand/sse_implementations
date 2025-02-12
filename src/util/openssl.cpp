#include <openssl/err.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include "openssl.h"

void handleOpenSslErrors() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

// PRF implemented with HMAC-SHA512, as done in Private Practical Range Search Revisited
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
