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


EncIndex PiBasClient::buildIndex() {
    // iterate over dictionary and create map to track new keywords vs. keys
    // every time new keyword encountered, add to map, create the key and perform the things
    // every time existing keyword encountered, encrypt using key from map
    // is this more efficient than methodin paper?
    // should be since we don't need separate pass to find all kws
    // and that pass alone should take as long as my whole approach?
    // if time experimentally check both?

    // can skip create setp in paper by just using ordered map so we already have it sorted? and thus skips the list?
}
