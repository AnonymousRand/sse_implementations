// temp compilation instructions: g++ pi_bas.cpp -lcrypto -o a
#include "pi_bas.h"

#include <iostream>
#include <openssl/rand.h>
#include <string.h>

void PiBasServer::setEncIndex(EncIndex encIndex) {
    this->encIndex = encIndex;
}

std::vector<int> PiBasServer::search(QueryToken queryToken) {
    std::vector<int> results;
    ustring subkey1 = std::get<0>(queryToken);
    ustring subkey2 = std::get<1>(queryToken);
    int counter = 0;
    
    // for c = 0 until Get returns error
    while (true) {
        // d <- Get(D, F(K_1, c))
        ustring encIndexK = prf(subkey1, to_ustring(counter));
        auto it = this->encIndex.find(encIndexK);
        if (it == this->encIndex.end()) {
            break;
        }
        ustring encIndexV = it->second;
        // id <- Dec(K_2, d)
        ustring ptext = aesDecrypt(EVP_aes_256_ecb(), subkey2, encIndexV);
        results.push_back(from_ustring(ptext));

        counter++;
    }

    return results;
}

PiBasClient::PiBasClient(Db db) {
    this->db = db;
    // build set of unique keywords for use during `setup()`
    for (auto pair : db) {
        this->uniqueKws.insert(pair.second);
    }
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
        // todo use IV and randomized encryption
        // K_1 || K_2 <- F(K, w)
        ustring K = prf(this->key, to_ustring(kw));
        int subkeyLen = K.length() / 2;
        ustring subkey1 = K.substr(0, subkeyLen);
        ustring subkey2 = K.substr(subkeyLen, subkeyLen);
        
        unsigned int counter = 0;
        // for each id in DB(w) (evil O(n^2) if ~=n unique keywords)
        for (auto pair : this->db) {
            if (pair.second != kw) {
                continue;
            }
            // l <- F(K_1, c); d <- Enc(K_2, id)
            ustring encIndexK = prf(subkey1, to_ustring(counter));
            ustring encIndexV = aesEncrypt(EVP_aes_256_ecb(), subkey2, to_ustring(pair.first));
            counter++;
            // add (l, d) to list L (in lex order)
            // we add straight to dictionary here since we have ordered maps in C++
            encIndex[encIndexK] = encIndexV;
        }
    }

    return encIndex;
}

QueryToken PiBasClient::trpdr(int kw) {
    // the paper uses different notation for the key generation here vs. in `setup()`;
    // I'm fairly sure they meant the same thing
    ustring K = prf(this->key, to_ustring(kw));
    int subkeyLen = K.length() / 2;
    ustring subkey1 = K.substr(0, subkeyLen);
    ustring subkey2 = K.substr(subkeyLen, subkeyLen);
    return std::tuple<ustring, ustring>(subkey1, subkey2);
}
