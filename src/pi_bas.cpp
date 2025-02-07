#include "pi_bas.h"

////////////////////////////////////////////////////////////////////////////////
// PiBasServer
////////////////////////////////////////////////////////////////////////////////

std::vector<int> PiBasServer::search(QueryToken queryToken) {
    std::vector<int> results;
    ustring subkey1 = queryToken.first;
    ustring subkey2 = queryToken.second;
    int counter = 0;
    
    // for c = 0 until `Get` returns error
    while (true) {
        // d <- Get(D, F(K_1, c))
        ustring label = prf(subkey1, intToUStr(counter));
        auto it = this->encIndex.find(label);
        if (it == this->encIndex.end()) {
            break;
        }
        std::pair<ustring, ustring> encIndexV = it->second;
        ustring data = encIndexV.first;
        // id <- Dec(K_2, d)
        ustring iv = encIndexV.second;
        ustring ptext = aesDecrypt(EVP_aes_256_cbc(), subkey2, data, iv);
        results.push_back(ustrToInt(ptext));

        counter++;
    }
    return results;
}

////////////////////////////////////////////////////////////////////////////////
// PiBasClient
////////////////////////////////////////////////////////////////////////////////

PiBasClient::PiBasClient(Db db) : SseClient(db) {};

void PiBasClient::setup(int secParam) {
    unsigned char* key = new unsigned char[secParam];
    int res = RAND_priv_bytes(key, secParam);
    if (res != 1) {
        handleOpenSslErrors();
    }
    this->key = ucharptrToUstr(key, secParam);
    this->keyLen = secParam;
    delete[] key;
}

EncIndex PiBasClient::buildIndex() {
    EncIndex encIndex;
    for (KwRange kwRange : this->uniqueKwRanges) {
        // K_1 || K_2 <- F(K, w)
        ustring K = prf(this->key, kwRangeToUstr(kwRange));
        int subkeyLen = K.length() / 2;
        ustring subkey1 = K.substr(0, subkeyLen);
        ustring subkey2 = K.substr(subkeyLen, subkeyLen);
        
        unsigned int counter = 0;
        // for each id in DB(w) (evil O(n^2) if ~=n unique keywords)
        // todo this is unacceptable? 2^20 size database never seems to finish; make sure it's stuck on buildIndex
        // maybe actually just make sse objects store plaintext map keywords -> ids? build in constructor?
        for (auto pair : this->db) {
            if (pair.second != kwRange) {
                continue;
            }
            // l <- F(K_1, c); d <- Enc(K_2, id)
            ustring label = prf(subkey1, intToUStr(counter));
            ustring iv = genIv();
            ustring data = aesEncrypt(EVP_aes_256_cbc(), subkey2, intToUStr(pair.first), iv);
            counter++;
            // add (l, d) to list L (in lex order); we add straight to dictionary since we have ordered maps in C++
            // also store IV in plain along with encrypted value
            encIndex[label] = std::pair<ustring, ustring> {data, iv};
        }
    }

    return encIndex;
}

QueryToken PiBasClient::trpdr(KwRange kwRange) {
    // the paper uses different notation for the key generation here vs. in `setup()`;
    // I'm fairly sure they meant the same thing
    ustring K = prf(this->key, kwRangeToUstr(kwRange));
    int subkeyLen = K.length() / 2;
    ustring subkey1 = K.substr(0, subkeyLen);
    ustring subkey2 = K.substr(subkeyLen, subkeyLen);
    return std::pair<ustring, ustring> {subkey1, subkey2};
}
