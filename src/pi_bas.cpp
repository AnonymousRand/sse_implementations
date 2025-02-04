// temp compilation instructions: g++ pi_bas.cpp -lcrypto -o a
#include "pi_bas.h"

#include <iostream>
#include <openssl/rand.h>


PiBasClient::PiBasClient(Db db) {
    this->db = db;
}


void PiBasClient::setup(int secParam) {
    unsigned char* key = new unsigned char[secParam + 1];
    int res = RAND_priv_bytes(key, secParam);
    if (res != 1) {
        std::cerr << "Error: `RAND_priv_bytes()` in `PiBasClient.set()` returned result " << res << " :/" << std::endl;
        exit(EXIT_FAILURE);
    }
    this->key = key;
}
