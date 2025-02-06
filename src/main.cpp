#include "pi_bas.h"
#include "util.h" // temp

#include <cmath>
#include <iostream>
#include <openssl/rand.h> // temp
#include <random>


void exp1(PiBasClient client, PiBasServer server) {
    client.setup(KEY_SIZE);
}


// temp
void gen_key(unsigned char key[KEY_SIZE]) {
    int rc = RAND_bytes(key, KEY_SIZE);
    if (rc != 1) {
      throw std::runtime_error("RAND_bytes key failed");
    }
}


int main() {
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
    //exp1(piBasClient, piBasServer);

    // experiment 2: fixed range size and vary db sizes
    
    // temp
    //gen_key(key);
    unsigned char key[64] = {
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
        0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77
    };
    std::cout << "key is " << key << std::endl;
    unsigned char* output;
    output = prf(key, 64, strToUCharPtr("to be or not to be"), 18);
    BIO_dump_fp(stdout, output, 512 / 8);
    //unsigned char key[KEY_SIZE];
    //unsigned char ctext[128];
    //unsigned char ptext[128];
    //int ctextLen = aesEncrypt(EVP_aes_256_ecb(), strToUCharPtr("hi"), 2, ctext, key);
    //BIO_dump_fp(stdout, ctext, ctextLen);
    //aesDecrypt(EVP_aes_256_ecb(), ctext, ctextLen, ptext, key);
    //std::cout << ptext << std::endl;
}
