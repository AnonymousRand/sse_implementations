#pragma once

#include <openssl/evp.h>

#include "util.h"

ustring genKey(int keyLen);
ustring genIv(int ivLen);

ustring findHash(const EVP_MD* hashFunc, int hashOutputLen, ustring input);
ustring prf(ustring key, ustring input);

ustring encrypt(const EVP_CIPHER* cipher, ustring key, ustring ptext, ustring iv = ustring());
ustring decrypt(const EVP_CIPHER* cipher, ustring key, ustring ctext, ustring iv = ustring());
