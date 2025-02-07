// temp compilation instructions: g++ pi_bas.cpp -lcrypto -o a
#include "pi_bas.h"

#include <iostream>
#include <openssl/rand.h>
#include <string.h>

void PiBasServer::setEncIndex(EncIndex encIndex) {
    this->encIndex = encIndex;
}

std::vector<int> PiBasServer::search(QueryToken queryToken) {
    int counter = 0;
    
    while (true) {
        ustring encIndexK = prf(
    }
}

PiBasClient::PiBasClient(Db db) {
    this->db = db;
    // build set of unique keywords for use during `setup()`
    for (auto pair : db) {
        this->uniqueKws.insert(pair.second);
    }
}

PiBasClient::~PiBasClient() {
}

void PiBasClient::setup(int secParam) {
    unsigned char* key = new unsigned char[secParam];
    int res = RAND_priv_bytes(key, secParam);
    if (res != 1) {
        std::cerr << "Error: `RAND_priv_bytes()` in `PiBasClient.set()` returned result " << res << " :/" << std::endl;
        exit(EXIT_FAILURE);
    }
    this->key = ustring(key);
    this->keyLen = secParam;
    delete[] key;
}

EncIndex PiBasClient::buildIndex() {
    EncIndex encIndex;
    for (int kw : this->uniqueKws) {
        // K_1 || K_2 <- F(K, w)
        ustring K = prf(this->key, to_ustring(kw));
        int subkeyLen = K.length() / 2;
        ustring subkey1 = K.substr(subkeyLen);
        ustring subkey2 = K.substr(subkeyLen, K.length());
        
        // initialize counter c <- 0
        unsigned int counter = 0;
        // for each id in DB(w) (evil O(n^2) if ~=n unique keywords)
        for (auto pair : this->db) {
            if (pair.second != kw) {
                continue;
            }
            
            // l <- F(K_1, c); d <- Enc(K_2, id); c++
            ustring encIndexK = prf(subkey1.c_str(), to_ustring(counter));
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

QueryToken PiBasClient::trpdr(int kw) {
    ustring subkey1 = prf(this->key, to_ustring(1) + to_ustring(kw));
    ustring subkey2 = prf(this->key, to_ustring(2) + to_ustring(kw));
    return std::tuple<ustring, ustring>(subkey1, subkey2);
}
