// temp compilation instructions: g++ pi_bas.cpp -lcrypto -o a
#include "pi_bas.h"

#include <iostream>
#include <openssl/rand.h>


PiBasClient::PiBasClient(Db db) {
    this->db = db;
    // build set of unique keywords for use during `setup()`
    for (auto pair : db) {
        this->uniqueKws.insert(pair.second);
    }
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


EncIndex PiBasClient::buildIndex() {
    // can skip create setp in paper by just using ordered map so we already have it sorted? and thus skips the list?
    for (int kw : this->uniqueKws) {
        // AES-ECB used as PRF/PRP for simplicitly (hence restriction on key lengths)
    }
}
