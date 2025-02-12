#include <openssl/evp.h>

#include "util.h"

void handleOpenSslErrors();

ustring prf(ustring key, ustring input);

ustring genIv();
ustring aesEncrypt(const EVP_CIPHER* cipher, ustring key, ustring ptext, ustring iv = ustring());
ustring aesDecrypt(const EVP_CIPHER* cipher, ustring key, ustring ctext, ustring iv = ustring());
