#include "pi_bas.h"
#include "util.h" // temp

#include <cmath>
#include <iostream>
#include <random>


void exp1(int secParam, PiBasClient client, PiBasServer server) {
    client.setup(secParam);
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
    unsigned char* key = strToUCharPtr("11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");
    std::tuple<unsigned char*, int> c = aesEncrypt(EVP_aes_256_ecb(), strToUCharPtr("hi"), 2, key, 256);
    std::cout << std::get<0>(c) << " length " << std::get<1>(c) << std::endl;
    unsigned char* m = aesDecrypt(EVP_aes_256_ecb(), std::get<0>(c), std::get<1>(c), key);
    std::cout << m << std::endl;
}
