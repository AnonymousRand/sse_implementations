#pragma once


#include <openssl/evp.h>

#include "utils.h"


ustring genKey(int keyLen);
ustring genIv(int ivLen);

ustring findHash(const EVP_MD* hashFunc, int hashOutputLen, const ustring& input);
ustring prf(const ustring& key, const ustring& input);

ustring encrypt(const EVP_CIPHER* cipher, const ustring& key, const ustring& ptext, const ustring& iv = ustring());
// pad `ptext` to `targetLenBytes` before encrypting
ustring padAndEncrypt(
    const EVP_CIPHER* cipher, const ustring& key, const ustring& ptext, const ustring& iv, int targetLenBytes
);
ustring decrypt(const EVP_CIPHER* cipher, const ustring& key, const ustring& ctext, const ustring& iv = ustring());
// remove trailing padding generate by `padAndEncrypt()` after decrypting
ustring decryptAndUnpad(const EVP_CIPHER* cipher, const ustring& key, const ustring& ctext, const ustring& iv);
