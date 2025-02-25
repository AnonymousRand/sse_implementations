#pragma once

#include <openssl/evp.h>

#include "util.h"

ustring genKey(int keyLen);
ustring genIv(int ivLen);

ustring findHash(const EVP_MD* hashFunc, int hashOutputLen, const ustring& input);
ustring prf(const ustring& key, const ustring& input);

ustring encrypt(const EVP_CIPHER* cipher, const ustring& key, const ustring& ptext, const ustring& iv = ustring());
ustring decrypt(const EVP_CIPHER* cipher, const ustring& key, const ustring& ctext, const ustring& iv = ustring());
