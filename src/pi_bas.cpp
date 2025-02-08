#include <set>

#include "pi_bas.h"

////////////////////////////////////////////////////////////////////////////////
// PiBasClient
////////////////////////////////////////////////////////////////////////////////

PiBasClient::PiBasClient(Db db) {
    this->db = db;
    if (this->db.empty()) {
        std::cerr << "Error: `db` passed to `PiBasClient.PiBasClient()` is empty :/" << std::endl;
        exit(EXIT_FAILURE);
    }
}

EncIndex PiBasClient::buildIndexInner(Db db) {
    EncIndex encIndex;

    // generate (plaintext) index of keywords to documents mapping and list of unique keywords
    std::map<KwRange, std::vector<Id>> index;
    std::set<KwRange> uniqueKwRanges;
    for (auto pair : db) {
        Id id = pair.first;
        KwRange kwRange = pair.second;

        if (index.count(kwRange) == 0) {
            index[kwRange] = std::vector<Id> {id};
        } else {
            index[kwRange].push_back(id);
        }
        uniqueKwRanges.insert(kwRange); // `std::set` will not insert duplicate elements
    }

    // for each w in W
    for (KwRange kwRange : uniqueKwRanges) {
        // K_1 || K_2 <- F(K, w)
        ustring K = prf(this->key, kwRangeToUstr(kwRange));
        int subkeyLen = K.length() / 2;
        ustring subkey1 = K.substr(0, subkeyLen);
        ustring subkey2 = K.substr(subkeyLen, subkeyLen);
        
        unsigned int counter = 0;
        // for each id in DB(w)
        auto docsWithKwRangeIt = index.find(kwRange);
        if (docsWithKwRangeIt == index.end()) {
            continue;
        }
        for (Id id : docsWithKwRangeIt->second) {
            // l <- F(K_1, c); d <- Enc(K_2, id)
            ustring label = prf(subkey1, intToUstr(counter));
            ustring iv = genIv();
            ustring data = aesEncrypt(EVP_aes_256_cbc(), subkey2, intToUstr(id), iv);
            counter++;
            // add (l, d) to list L (in lex order); we add straight to dictionary since we have ordered maps in C++
            // also store IV in plain along with encrypted value
            encIndex[label] = std::pair<ustring, ustring> {data, iv};
        }
    }

    return encIndex;
}

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
    // can't use `this->db` as default param as it's evaluated at runtime
    return this->buildIndexInner(this->db);
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
        ustring label = prf(subkey1, intToUstr(counter));
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

void PiBasServer::setEncIndex(EncIndex encIndex) {
    this->encIndex = encIndex;
}
