#include "utils/crypto.h"

#include <cstdlib>
#include <format>
#include <iostream>

#include <openssl/err.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include "utils/debug.h"
#include "utils/str.h"
#include "utils/types/ustring.h"


// thanks to
// https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption#C.2B.2B_Programs,
// https://wiki.openssl.org/index.php/EVP_Message_Digests,
// and https://stackoverflow.com/a/34624592 for good reference code


namespace {


void handleErrors(const std::string& beginText = "") {
    std::cerr << beginText;
    ERR_print_errors_fp(stderr);
    std::exit(EXIT_FAILURE);
}


} // anonymous namespace


//==============================================================================
// `utils::crypto`
//==============================================================================


namespace utils::crypto {


ustring genKey(int keyLen) {
    uchar* key = new uchar[keyLen];
    int res = RAND_priv_bytes(key, keyLen);
    if (res != 1) {
        handleErrors("Error: utils::crypto::genKey(): ");
    }
    ustring ustrKey(key, keyLen);
    delete[] key;
    return ustrKey;
}


ustring genIv(int ivLen) {
    uchar* iv = new uchar[ivLen];
    int res = RAND_bytes(iv, ivLen);
    if (res != 1) {
        handleErrors("Error: utils::crypto::genIv(): ");
    }
    ustring ustrIv(iv, ivLen);
    delete[] iv;
    return ustrIv;
}



ustring hash(const ustring& input, const EVP_MD* hashFunc, int hashOutputLen) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        handleErrors(
            std::format("Error: utils::crypto::hash() creating context (input is \"{}\"): ", input)
        );
    }

    // initialize hash
    if (EVP_DigestInit_ex(ctx, hashFunc, NULL) != 1) {
        handleErrors(
            std::format("Error: utils::crypto::hash() initializing hash (input is \"{}\"): ", input)
        );
    }

    // perform hash
    unsigned int hashLen;
    ustring hash;
    hash.resize(hashOutputLen);
    if (EVP_DigestUpdate(ctx, input.data(), input.length()) != 1) {
        handleErrors(
            std::format("Error: utils::crypto::hash() performing hash (input is \"{}\"): ", input)
        );
    }

    // finalize hash by outputting the digest
    if (EVP_DigestFinal_ex(ctx, hash.data(), &hashLen) != 1) {
        handleErrors(
            std::format("Error: utils::crypto::hash() finalizing hash (input is \"{}\"): ", input)
        );
    }

    EVP_MD_CTX_free(ctx);
    hash.resize(hashLen);
    return hash;
}


// PRF implemented with HMAC-SHA256
ustring prf(const ustring& key, const ustring& input) {
    unsigned int outputLen;
    uchar* output = HMAC(
        HASH_FUNC, key.data(), key.length(), input.data(), input.length(), nullptr, &outputLen
    );
    return ustring(output, outputLen);
}


ustring encrypt(
    const ustring& key, const ustring& ptext, const ustring& iv, const EVP_CIPHER* cipher
) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        handleErrors(
            std::format(
                "Error: utils::crypto::encrypt() creating context (ptext is \"{}\"): ", ptext
            )
        );
    }

    // initialize encryption
    const uchar* ucharIv;
    if (iv.length() > 0) {
        ucharIv = iv.data();
    } else {
        ucharIv = nullptr;
    }
    if (EVP_EncryptInit_ex(ctx, cipher, nullptr, key.data(), ucharIv) != 1) {
        handleErrors(
            std::format(
                "Error: utils::crypto::encrypt() initializing encryption (ptext is \"{}\"): ", ptext
            )
        );
    }

    // perform encryption
    int ctextLen1, ctextLen2;
    ustring ctext;
    ctext.resize(ptext.length() + BLOCK_SIZE); // need to allocate worst-case size first
    if (EVP_EncryptUpdate(ctx, ctext.data(), &ctextLen1, ptext.data(), ptext.length()) != 1) {
        handleErrors(
            std::format(
                "Error: utils::crypto::encrypt() performing encryption (ptext is \"{}\"): ", ptext
            )
        );
    }

    // finalize encryption (deal with last partial block)
    if (EVP_EncryptFinal_ex(ctx, ctext.data() + ctextLen1, &ctextLen2) != 1) {
        handleErrors(
            std::format(
                "Error: utils::crypto::encrypt() finalizing encryption (ptext is \"{}\"): ", ptext
            )
        );
    }

    EVP_CIPHER_CTX_free(ctx);
    ctext.resize(ctextLen1 + ctextLen2);
    return ctext;
}


ustring padAndEncrypt(
    const ustring& key, ustring ptext, const ustring& iv, int targetLen, const EVP_CIPHER* cipher
) {
    DEBUG_ONLY({
        if (ptext.length() > targetLen) {
            std::cerr << "Error: padAndEncrypt(): plaintext \"" << ptext
                      << "\" of length " << ptext.length() << " bytes is too long! "
                      << "(want " << targetLen << " bytes)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });
    utils::str::padStrEnd(ptext, targetLen);
    return encrypt(key, ptext, iv, cipher);
}


ustring decrypt(
    const ustring& key, const ustring& ctext, const ustring& iv, const EVP_CIPHER* cipher
) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        handleErrors(
            std::format(
                "Error: utils::crypto::decrypt() creating context (ctext is \"{}\"): ",
                utils::debug::ustrToHex(ctext)
            )
        );
    }

    // initialize decryption
    const uchar* ucharIv;
    if (iv.length() > 0) {
        ucharIv = iv.data();
    } else {
        ucharIv = nullptr;
    }
    if (EVP_DecryptInit_ex(ctx, cipher, nullptr, key.data(), ucharIv) != 1) {
        handleErrors(
            std::format(
                "Error: utils::crypto::decrypt() initializing decryption (ctext is \"{}\"): ",
                utils::debug::ustrToHex(ctext)
            )
        );
    }

    // perform decryption
    int ptextLen1, ptextLen2;
    ustring ptext;
    ptext.resize(ctext.length());
    if (EVP_DecryptUpdate(ctx, ptext.data(), &ptextLen1, ctext.data(), ctext.length()) != 1) {
        handleErrors(
            std::format(
                "Error: utils::crypto::decrypt() performing decryption (ctext is \"{}\"): ",
                utils::debug::ustrToHex(ctext)
            )
        );
    }

    // finalize decryption (deal with last partial block)
    if (EVP_DecryptFinal_ex(ctx, ptext.data() + ptextLen1, &ptextLen2) != 1) {
        handleErrors(
            std::format(
                "Error: utils::crypto::decrypt() finalizing decryption (ctext is \"{}\"): ",
                utils::debug::ustrToHex(ctext)
            )
        );
    }

    EVP_CIPHER_CTX_free(ctx);
    ptext.resize(ptextLen1 + ptextLen2);
    return ptext;
}


} // namespace `utils::crypto`
