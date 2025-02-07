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
        unsigned char* K = new unsigned char[512 / 8];
        unsigned char* sKw = new unsigned char[countDigits(kw)];
        int sKwLen = intToUCharPtr(kw, sKw);
        int subkeyLen = prf(this->key, this->secParam, sKw, sKwLen, K) / 2;
        unsigned char* subkey1 = K;
        unsigned char* subkey2 = K + subkeyLen;
        
        // initialize counter c <- 0
        unsigned int counter = 0;
        // for each id in DB(w)
        for (auto pair : this->db) {
            if (pair.second != kw) {
                continue;
            }
            
            // l <- F(K_1, c); d <- Enc(K_2, id); c++
            unsigned char* sCounter = new unsigned char[countDigits(counter)];
            int sCounterLen = intToUCharPtr(counter, sCounter);
            unsigned char* encIndexK = new unsigned char[sCounterLen + BLOCK_SIZE];
            int encIndexKLen = prf(subkey1, subkeyLen, sCounter, sCounterLen, encIndexK); // todo need to store these lengths as well for decryption?

            int id = pair.first;
            unsigned char* sId = new unsigned char[countDigits(id)];
            int sIdLen = intToUCharPtr(id, sId);
            unsigned char* encIndexV = new unsigned char[sIdLen + BLOCK_SIZE];
            int encIndexVLen = aesEncrypt(subkey2, EVP_aes_256_ecb(), sId, sIdLen, encIndexV);

            counter++;

            // add (l, d) to list L (in lex order)
            // note we add straight to dictionary here since we have ordered maps in C++
            if (encIndex.count(encIndexK) == 0) {
                encIndex[encIndexK] = std::vector<unsigned char*> {encIndexV};
            } else {
                encIndex[encIndexK].push_back(encIndexV);
            }

            delete[] sId, sCounter, encIndexK, encIndexV;
        }

        delete[] K, sKw;
    }

    return encIndex;
}
