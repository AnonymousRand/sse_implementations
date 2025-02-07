// temp compilation instructions: g++ pi_bas.cpp -lcrypto -o a
#include "pi_bas.h"

#include <iostream>
#include <openssl/rand.h>
#include <string.h>

PiBasClient::PiBasClient(Db db) {
    this->db = db;
    // build set of unique keywords for use during `setup()`
    for (auto pair : db) {
        this->uniqueKws.insert(pair.second);
    }
    this->key = nullptr;
}

PiBasClient::~PiBasClient() {
    if (this->key != nullptr) {
        delete[] this->key;
    }
}

void PiBasClient::setup(int secParam) {
    this->secParam = secParam;
    unsigned char* key = new unsigned char[secParam];
    int res = RAND_priv_bytes(key, secParam);
    if (res != 1) {
        std::cerr << "Error: `RAND_priv_bytes()` in `PiBasClient.set()` returned result " << res << " :/" << std::endl;
        exit(EXIT_FAILURE);
    }
    this->key = key;
}

EncIndex PiBasClient::buildIndex() {
    // can skip create setp in paper by just using ordered map so we already have it sorted? and thus skips the list?
    // evil O(n^2) if ~=n unique keywords
    EncIndex encIndex;
    for (int kw : this->uniqueKws) {
        // K_1 || K_2 <- F(K, w)
        ustring K = prf(this->key, this->secParam, to_ustring(kw));
        int subkeyLen = K.length() / 2;
        ustring subkey1 = K.substr(subkeyLen);
        ustring subkey2 = K.substr(subkeyLen, K.length());
        
        // initialize counter c <- 0
        unsigned int counter = 0;
        // for each id in DB(w)
        for (auto pair : this->db) {
            if (pair.second != kw) {
                continue;
            }
            
            // l <- F(K_1, c); d <- Enc(K_2, id); c++
            ustring encIndexK = prf(subkey1.c_str(), subkeyLen, to_ustring(counter)); // todo need to store these lengths as well for decryption? if not using ustring that is
            ustring encIndexV = aesEncrypt(EVP_aes_256_ecb(), subkey2.c_str(), to_ustring(pair.first));
            counter++;

            // add (l, d) to list L (in lex order)
            // we add straight to dictionary here since we have ordered maps in C++
            if (encIndex.count(encIndexK) == 0) {
                encIndex[encIndexK] = std::vector<ustring> {encIndexV};
            } else {
                encIndex[encIndexK].push_back(encIndexV);
            }
        }
    }

    return encIndex;
}

QueryToken trpdr(int kw) {
    ustring key1 =
}
