// temp compilation instructions: g++ pi_bas.cpp -lcrypto -o a
#include "pi_bas.h"

#include <iostream>
#include <string.h>

std::vector<int> PiBasServer::search(QueryToken queryToken) {
    std::vector<int> results;
    ustring subkey1 = std::get<0>(queryToken);
    ustring subkey2 = std::get<1>(queryToken);
    int counter = 0;
    
    // for c = 0 until `Get` returns error
    while (true) {
        // d <- Get(D, F(K_1, c))
        ustring encIndexK = prf(subkey1, to_ustring(counter));
        auto it = this->encIndex.find(encIndexK);
        if (it == this->encIndex.end()) {
            break;
        }
        ustring encIndexV = std::get<0>(it->second);
        // id <- Dec(K_2, d)
        ustring iv = std::get<1>(it->second);
        ustring ptext = aesDecrypt(EVP_aes_256_cbc(), subkey2, encIndexV, iv);
        results.push_back(from_ustring(ptext));

        counter++;
    }
    return results;
}

PiBasClient::PiBasClient(Db db) : SseClient(db) {};

void PiBasClient::setup(int secParam) {
    unsigned char* key = new unsigned char[secParam];
    int res = RAND_priv_bytes(key, secParam);
    if (res != 1) {
        handleOpenSslErrors();
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
            ustring iv = genIv();
            ustring encIndexV = aesEncrypt(EVP_aes_256_cbc(), subkey2, to_ustring(pair.first), iv);
            counter++;
            // add (l, d) to list L (in lex order); we add straight to dictionary since we have ordered maps in C++
            // also store IV in plain along with encrypted value
            encIndex[encIndexK] = std::tuple<ustring, ustring> {encIndexV, iv};
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
