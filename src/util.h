#include <openssl/err.h>
#include <openssl/evp.h>
#include <string>
#include <tuple>


// creating `intToUCharPtr(int n)` that returns `strToUCharPtr(std::to_string(n))` has weird memory issues?
// it screws up the value of the return back in the caller (it's correct in the callee)
unsigned char* strToUCharPtr(std::string s);

void handleOpenSslErrors();
std::tuple<unsigned char*, int> aesEncrypt(const EVP_CIPHER* cipher, unsigned char* plaintext, int plaintextLen, unsigned char* key, int blockSize);
unsigned char* aesDecrypt(const EVP_CIPHER* cipher, unsigned char* ciphertext, int ciphertextLen, unsigned char* key);
