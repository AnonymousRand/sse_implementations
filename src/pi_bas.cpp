#include <set>

#include "pi_bas.h"

////////////////////////////////////////////////////////////////////////////////
// PiBasClient
////////////////////////////////////////////////////////////////////////////////

ustring PiBasClient::setup(int secParam) {
    unsigned char* key = new unsigned char[secParam];
    int res = RAND_priv_bytes(key, secParam);
    if (res != 1) {
        handleOpenSslErrors();
    }
    ustring ustrKey = toUstr(key, secParam);
    delete[] key;
    return ustrKey;
}

template <typename DbDocType, typename DbKwType>
EncInd PiBasClient::buildIndex(ustring key, Db<std::tuple<DbDocType, DbKwType>> db) {
    EncInd encInd;

    // generate (plaintext) index of keywords to documents/ids mapping and list of unique keywords
    std::map<DbKwType, std::vector<DbDocType>> index;
    std::set<DbKwType> uniqueKws;
    for (auto entry : db) {
        DbDocType doc = std::get<0>(entry);
        DbKwType kw = std::get<1>(entry);

        if (index.count(kw) == 0) {
            index[kw] = std::vector<DbDocType> {doc};
        } else {
            index[kw].push_back(doc);
        }
        uniqueKws.insert(kw); // `std::set` will not insert duplicate elements
    }

    // for each w in W
    for (DbKwType kw : uniqueKws) {
        // K_1 || K_2 <- F(K, w)
        ustring K = prf(key, toUstr(kw));
        int subkeyLen = K.length() / 2;
        ustring subkey1 = K.substr(0, subkeyLen);
        ustring subkey2 = K.substr(subkeyLen, subkeyLen);
        
        unsigned int counter = 0;
        // for each id in DB(w)
        auto itDocsWithSameKw = index.find(kw);
        if (itDocsWithSameKw == index.end()) {
            continue;
        }
        for (DbDocType doc : itDocsWithSameKw->second) {
            // l <- F(K_1, c); d <- Enc(K_2, id)
            ustring label = prf(subkey1, toUstr(counter));
            ustring iv = genIv();
            // todo need way to encode doc for srci index1
            // surely just another overload works due to sfinae right?
            ustring data = aesEncrypt(EVP_aes_256_cbc(), subkey2, toUstr(doc), iv);
            counter++;
            // add (l, d) to list L (in lex order); we add straight to dictionary since we have ordered maps in C++
            // also store IV in plain along with encrypted value
            encInd[label] = std::pair<ustring, ustring> {data, iv};
        }
    }

    return encInd;
}

QueryToken PiBasClient::trpdr(ustring key, KwRange kwRange) {
    // the paper uses different notation for the key generation here vs. in `setup()`;
    // I'm fairly sure they meant the same thing
    ustring K = prf(key, toUstr(kwRange));
    int subkeyLen = K.length() / 2;
    ustring subkey1 = K.substr(0, subkeyLen);
    ustring subkey2 = K.substr(subkeyLen, subkeyLen);
    return std::pair<ustring, ustring> {subkey1, subkey2};
}

////////////////////////////////////////////////////////////////////////////////
// PiBasServer
////////////////////////////////////////////////////////////////////////////////

std::vector<int> PiBasServer::search(EncInd encInd, QueryToken queryToken) {
    std::vector<int> results;
    ustring subkey1 = queryToken.first;
    ustring subkey2 = queryToken.second;
    int counter = 0;
    
    // for c = 0 until `Get` returns error
    while (true) {
        // d <- Get(D, F(K_1, c))
        ustring label = prf(subkey1, toUstr(counter));
        auto it = encInd.find(label);
        if (it == encInd.end()) {
            break;
        }
        std::pair<ustring, ustring> encIndV = it->second;
        ustring data = encIndV.first;
        // id <- Dec(K_2, d)
        ustring iv = encIndV.second;
        ustring ptext = aesDecrypt(EVP_aes_256_cbc(), subkey2, data, iv);
        results.push_back(ustrToInt(ptext));

        counter++;
    }

    return results;
}
