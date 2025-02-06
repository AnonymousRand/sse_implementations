#include "pi_bas.h"
#include "util.h" // temp

#include <cmath>
#include <iostream>
#include <openssl/rand.h> // temp
#include <random>


void exp1(int secParam, PiBasClient client, PiBasServer server) {
    client.setup(secParam);
}


// temp
void gen_key(unsigned char key[KEY_SIZE]) {
    int rc = RAND_bytes(key, KEY_SIZE);
    if (rc != 1)
      throw std::runtime_error("RAND_bytes key failed");
}


int main() {
    int secParam = 256;

    // experiment 1: db of size 2^20 and vary range sizes
    Db db;
    std::random_device dev;
    std::mt19937 rand(dev());
    std::uniform_int_distribution<int> dist(pow(2, 20));
    for (int i = 0; i < pow(2, 20); i++) {
        db[i] = dist(rand);
    }

    PiBasClient piBasClient = PiBasClient(db);
    PiBasServer piBasServer = PiBasServer();
    //exp1(secParam, piBasClient, piBasServer);

    // experiment 2: fixed range size and vary db sizes
    
    // temp
    // todo incorrect encryption/decyprtion error
    // null terminator?
    unsigned char key[KEY_SIZE];
    gen_key(key);
    openSslStr c = aesEncrypt(EVP_aes_256_ecb(), "hi", key);
    //BIO_dump_fp(stdout, std::get<0>(c), std::get<1>(c));
    openSslStr m = aesDecrypt(EVP_aes_256_ecb(), c, key);
    std::cout << m << std::endl;
}
