#include <set>

#include "pi_bas.h"

////////////////////////////////////////////////////////////////////////////////
// Client
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

EncInd PiBasClient::buildIndex(ustring key, Db<> db) {
    return this->buildIndexGeneric(key, db);
}

QueryToken PiBasClient::trpdr(ustring key, KwRange kwRange) {
    return this->trpdrGeneric(key, kwRange);
}

template <typename DbDocType, typename DbKwType>
EncInd PiBasClient::buildIndexGeneric(ustring key, Db<DbDocType, DbKwType> db) {
    // generate (plaintext) index of keywords to documents/ids mapping and list of unique keywords
    std::map<DbKwType, std::vector<DbDocType>> index;
    std::set<DbKwType> uniqueKws;
    for (auto entry : db) {
        DbDocType dbDoc = std::get<0>(entry);
        DbKwType dbKw = std::get<1>(entry);

        if (index.count(dbKw) == 0) {
            index[dbKw] = std::vector<DbDocType> {dbDoc};
        } else {
            index[dbKw].push_back(dbDoc);
        }
        uniqueKws.insert(dbKw); // `std::set` will not insert duplicate elements
    }

    EncInd encInd;
    // for each w in W
    for (DbKwType dbKw : uniqueKws) {
        // K_1 || K_2 <- F(K, w)
        ustring K = prf(key, toUstr(dbKw));
        int subkeyLen = K.length() / 2;
        ustring subkey1 = K.substr(0, subkeyLen);
        ustring subkey2 = K.substr(subkeyLen, subkeyLen);
        
        unsigned int counter = 0;
        // for each id in DB(w)
        auto itDocsWithSameKw = index.find(dbKw);
        for (DbDocType dbDoc : itDocsWithSameKw->second) {
            // l <- F(K_1, c); d <- Enc(K_2, id)
            ustring label = prf(subkey1, toUstr(counter));
            ustring iv = genIv();
            ustring data = aesEncrypt(EVP_aes_256_cbc(), subkey2, dbDoc.encode(), iv);
            counter++;
            // add (l, d) to list L (in lex order); we add straight to dictionary since we have ordered maps in C++
            // also store IV in plain along with encrypted value
            encInd[label] = std::pair<ustring, ustring> {data, iv};
        }
    }

    return encInd;
}

// todo see if can move these to srci where needed
template EncInd PiBasClient::buildIndexGeneric(ustring key, Db<> db);
template EncInd PiBasClient::buildIndexGeneric(ustring key, Db<SrciDb1Doc, KwRange> db);
template EncInd PiBasClient::buildIndexGeneric(ustring key, Db<Id, IdRange> db);

template <typename RangeType>
QueryToken PiBasClient::trpdrGeneric(ustring key, Range<RangeType> range) {
    // the paper uses different notation for the key generation here vs. in `setup()`;
    // but I'm fairly sure they meant the same thing, otherwise it doesn't work
    ustring K = prf(key, toUstr(range));
    int subkeyLen = K.length() / 2;
    ustring subkey1 = K.substr(0, subkeyLen);
    ustring subkey2 = K.substr(subkeyLen, subkeyLen);
    return QueryToken {subkey1, subkey2};
}

template QueryToken PiBasClient::trpdrGeneric(ustring key, Range<Id> range);
template QueryToken PiBasClient::trpdrGeneric(ustring key, Range<Kw> range);

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

std::vector<Id> PiBasServer::search(EncInd encInd, QueryToken queryToken) {
    return this->searchGeneric(encInd, queryToken);
}

template <typename DbDocType>
std::vector<DbDocType> PiBasServer::searchGeneric(EncInd encInd, QueryToken queryToken) {
    std::vector<DbDocType> results;
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
        results.push_back(DbDocType::decode(ptext));

        counter++;
    }

    return results;
}

template std::vector<Id> PiBasServer::searchGeneric(EncInd encInd, QueryToken queryToken);
template std::vector<SrciDb1Doc> PiBasServer::searchGeneric(EncInd encInd, QueryToken queryToken);
