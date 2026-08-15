#include "utils/crypto.h"

#include <cstdlib>
#include <iostream>

#include <openssl/err.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include "utils/ustring.h"


// thanks to
// https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption#C.2B.2B_Programs,
// https://wiki.openssl.org/index.php/EVP_Message_Digests,
// and https://stackoverflow.com/a/34624592 for good reference code


namespace {


void handleErrors() {
    ERR_print_errors_fp(stderr);
    std::exit(EXIT_FAILURE);
}


} // anonymous namespace


//==============================================================================
// crypto utils
//==============================================================================


namespace crypto {


ustring genKey(int keyLen) {
    uchar* key = new uchar[keyLen];
    int res = RAND_priv_bytes(key, keyLen);
    if (res != 1) {
        handleErrors();
    }
    ustring ustrKey = ::utils::toUstr(key, keyLen);
    delete[] key;
    return ustrKey;
}


ustring genIv(int ivLen) {
    uchar* iv = new uchar[ivLen];
    int res = RAND_bytes(iv, ivLen);
    if (res != 1) {
        handleErrors();
    }
    ustring ustrIv = ::utils::toUstr(iv, ivLen);
    delete[] iv;
    return ustrIv;
}


ustring hash(const EVP_MD* hashFunc, int hashOutputLen, const ustring& input) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        handleErrors();
    }

    // initialize hash
    if (EVP_DigestInit_ex(ctx, hashFunc, NULL) != 1) {
        handleErrors();
    }

    // perform hash
    unsigned int hashLen;
    ustring hash;
    hash.resize(hashOutputLen);
    if (EVP_DigestUpdate(ctx, &input[0], input.length()) != 1) {
        handleErrors();
    }

    // finalize hash by outputting the digest
    if (EVP_DigestFinal_ex(ctx, &hash[0], &hashLen) != 1) {
        handleErrors();
    }

    EVP_MD_CTX_free(ctx);
    hash.resize(hashLen);
    return hash;
}


// PRF implemented with HMAC-SHA512, as done in Demertzis'16
ustring prf(const ustring& key, const ustring& input) {
    unsigned int outputLen;
    uchar* output = HMAC(
        EVP_sha512(), &key[0], key.length(), &input[0], input.length(), nullptr, &outputLen
    );
    return ::utils::toUstr(output, outputLen);
}


ustring encrypt(
    const EVP_CIPHER* cipher, const ustring& key, const ustring& ptext, const ustring& iv
) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        handleErrors();
    }

    // initialize encryption
    const uchar* ucharIv;
    if (iv == ustring()) {
        ucharIv = nullptr;
    } else {
        ucharIv = &iv[0];
    }
    if (EVP_EncryptInit_ex(ctx, cipher, nullptr, &key[0], ucharIv) != 1) {
        handleErrors();
    }

    // perform encryption
    int ctextLen1, ctextLen2;
    ustring ctext;
    ctext.resize(ptext.length() + BLOCK_SIZE); // need to allocate worst-case size first
    if (EVP_EncryptUpdate(ctx, &ctext[0], &ctextLen1, &ptext[0], ptext.length()) != 1) {
        handleErrors();
    }

    // finalize encryption (deal with last partial block)
    if (EVP_EncryptFinal_ex(ctx, &ctext[0] + ctextLen1, &ctextLen2) != 1) {
        handleErrors();
    }

    EVP_CIPHER_CTX_free(ctx);
    ctext.resize(ctextLen1 + ctextLen2);
    return ctext;
}


ustring padAndEncrypt(
    const EVP_CIPHER* cipher, const ustring& key, const ustring& ptext, const ustring& iv,
    int targetLenBytes
) {
    if (targetLenBytes < ptext.length()) {
        std::cerr << "Error: padAndEncrypt(): plaintext of length " << ptext.length()
                  << " bytes is too long! " << "(want " << targetLenBytes << " bytes)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    int amountToPad = targetLenBytes - ptext.length();
    ustring padding(amountToPad, '\0');
    return encrypt(cipher, key, ptext + padding, iv);
}


ustring decrypt(
    const EVP_CIPHER* cipher, const ustring& key, const ustring& ctext, const ustring& iv
) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        handleErrors();
    }

    // initialize decryption
    const uchar* ucharIv;
    if (iv == ustring()) {
        ucharIv = nullptr;
    } else {
        ucharIv = &iv[0];
    }
    if (EVP_DecryptInit_ex(ctx, cipher, nullptr, &key[0], ucharIv) != 1) {
        handleErrors();
    }

    // perform decryption
    int ptextLen1, ptextLen2;
    ustring ptext;
    ptext.resize(ctext.length());
    if (EVP_DecryptUpdate(ctx, &ptext[0], &ptextLen1, &ctext[0], ctext.length()) != 1) {
        handleErrors();
    }

    // finalize decryption (deal with last partial block)
    if (EVP_DecryptFinal_ex(ctx, &ptext[0] + ptextLen1, &ptextLen2) != 1) {
        handleErrors();
    }

    EVP_CIPHER_CTX_free(ctx);
    ptext.resize(ptextLen1 + ptextLen2);
    return ptext;
}


ustring decryptAndUnpad(
    const EVP_CIPHER* cipher, const ustring& key, const ustring& ctext, const ustring& iv
) {
    ustring ptext = decrypt(cipher, key, ctext, iv);
    int paddingStart;
    for (paddingStart = ptext.length() - 1; paddingStart >= 0; paddingStart--) {
        if (ptext[paddingStart] != '\0') {
            break;
        }
    }
    ptext.resize(paddingStart + 1); // (`+ 1` to add back first null terminator)
    return ptext;
}


} // namespace `crypto`
