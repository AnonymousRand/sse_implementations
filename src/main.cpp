#include "pi_bas.h"

#include <cmath>
#include <iostream>
#include <random>

void exp1(PiBasClient client, PiBasServer server) {
    client.setup(KEY_SIZE);
    EncIndex encIndex = client.buildIndex();
    std::cout << "----- Encrypted Index -----" << std::endl;
    for (auto pair : encIndex) {
        std::cout << pair.first << ": ";
        for (auto it : pair.second) {
            std::cout << *it << ", ";
        }
        std::cout << std::endl;
    }
}

int main() {
    // experiment 1: db of size 2^20 and vary range sizes
    Db db;
    const int dbSize = pow(2, 5);
    std::random_device dev;
    std::mt19937 rand(dev());
    std::uniform_int_distribution<int> dist(dbSize);
    std::cout << "----- Dataset -----" << std::endl;
    for (int i = 0; i < dbSize; i++) {
        db[i] = dist(rand);
        std::cout << i << ": " << db[i] << std::endl;
    }

    PiBasClient piBasClient = PiBasClient(db);
    PiBasServer piBasServer = PiBasServer();
    exp1(piBasClient, piBasServer);

    // experiment 2: fixed range size and vary db sizes
    
    // temp
    //unsigned char* org = (unsigned char*)"1000";
    //std::cout << "1: " << org << std::endl;
    //int n = 1000;
    //std::string s2 = std::to_string(n);
    //std::cout << "2: " << (unsigned char*)(s2.c_str()) << std::endl;
    //unsigned char* output = new unsigned char;
    //intToUCharPtr(1000, output);
    //output[4] = '\0';
    //std::cout << "3: " << output << std::endl;
    //delete[] output;
    //gen_key(key);
    //unsigned char key[64] = {
    //    0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
    //    0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
    //    0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
    //    0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
    //    0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
    //    0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
    //    0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
    //    0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77
    //};
    //std::cout << "key is " << key << std::endl;
    //unsigned char* output;
    //output = prf(key, 64, strToUCharPtr("to be or not to be"), 18);
    //BIO_dump_fp(stdout, output, 512 / 8);
    //unsigned char key[KEY_SIZE];
    //unsigned char ctext[128];
    //unsigned char ptext[128];
    //int ctextLen = aesEncrypt(EVP_aes_256_ecb(), strToUCharPtr("hi"), 2, ctext, key);
    //BIO_dump_fp(stdout, ctext, ctextLen);
    //aesDecrypt(EVP_aes_256_ecb(), ctext, ctextLen, ptext, key);
    //std::cout << ptext << std::endl;
}
