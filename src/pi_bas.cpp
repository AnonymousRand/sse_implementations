#include <unordered_set>

#include <openssl/rand.h>

#include "pi_bas.h"
#include "util/openssl.h"

////////////////////////////////////////////////////////////////////////////////
// API Functions
////////////////////////////////////////////////////////////////////////////////

template <typename DbDocType>
void PiBas<DbDocType>::setup(int secParam, const Db<DbDocType>& db) {
    this->key = this->genKey(secParam);
    this->encInd = this->buildIndex(this->key, db);
}

template <typename DbDocType>
std::vector<DbDocType> PiBas<DbDocType>::search(const KwRange& query) {
    QueryToken queryToken = this->genQueryToken(this->key, query);
    return this->serverSearch<DbDocType>(this->encInd, queryToken);
}

////////////////////////////////////////////////////////////////////////////////
// Non-API Functions
////////////////////////////////////////////////////////////////////////////////

template <typename DbDocType>
ustring PiBas<DbDocType>::genKey(int secParam) const {
    unsigned char* key = new unsigned char[secParam];
    int res = RAND_priv_bytes(key, secParam);
    if (res != 1) {
        handleOpenSslErrors();
    }
    ustring ustrKey = toUstr(key, secParam);
    delete[] key;
    return ustrKey;
}

template <typename DbDocType>
template <typename DbDocType2, typename DbKwType>
EncInd PiBas<DbDocType>::buildIndex(const ustring& key, const Db<DbDocType2, DbKwType>& db) const {
    // generate (plaintext) index of keywords to documents/ids mapping and list of unique keywords
    std::unordered_map<DbKwType, std::vector<DbDocType2>> index;
    std::unordered_set<DbKwType> uniqueKws;
    for (auto entry : db) {
        DbDocType2 dbDoc = std::get<0>(entry);
        DbKwType dbKw = std::get<1>(entry);

        if (index.count(dbKw) == 0) {
            index[dbKw] = std::vector {dbDoc};
        } else {
            index[dbKw].push_back(dbDoc);
        }
        uniqueKws.insert(dbKw); // `unordered_set` will not insert duplicate elements
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
        for (DbDocType2 dbDoc : itDocsWithSameKw->second) {
            // l <- F(K_1, c); d <- Enc(K_2, id)
            ustring label = prf(subkey1, toUstr(counter));
            ustring iv = genIv();
            ustring data = aesEncrypt(EVP_aes_256_cbc(), subkey2, dbDoc.encode(), iv);
            counter++;
            // add (l, d) to list L (in lex order); we add straight to dictionary since we have ordered maps in C++
            // also store IV in plain along with encrypted value
            encInd[label] = std::pair {data, iv};
        }
    }

    return encInd;
}

template <typename DbDocType>
template <typename RangeType>
QueryToken PiBas<DbDocType>::genQueryToken(const ustring& key, const Range<RangeType>& range) const {
    // the paper uses different notation for the key generation here vs. in `setup()`;
    // but I'm fairly sure they meant the same thing, otherwise it doesn't work
    ustring K = prf(key, toUstr(range));
    int subkeyLen = K.length() / 2;
    ustring subkey1 = K.substr(0, subkeyLen);
    ustring subkey2 = K.substr(subkeyLen, subkeyLen);
    return QueryToken {subkey1, subkey2};
}

template <typename DbDocType>
template <typename DbDocType2>
std::vector<DbDocType2> PiBas<DbDocType>::serverSearch(const EncInd& encInd, const QueryToken& queryToken) const {
    ustring subkey1 = queryToken.first;
    ustring subkey2 = queryToken.second;
    std::vector<DbDocType2> results;
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
        results.push_back(DbDocType2::decode(ptext));

        counter++;
    }

    return results;
}

////////////////////////////////////////////////////////////////////////////////
// Template Instantiations
////////////////////////////////////////////////////////////////////////////////

template class PiBas<Id>;

template EncInd PiBas<Id>::buildIndex(const ustring& key, const Db<>& db) const;
template EncInd PiBas<Id>::buildIndex(const ustring& key, const Db<SrciDb1Doc, KwRange>& db) const;
template EncInd PiBas<Id>::buildIndex(const ustring& key, const Db<Id, IdRange>& db) const;

template QueryToken PiBas<Id>::genQueryToken(const ustring& key, const IdRange& range) const;
template QueryToken PiBas<Id>::genQueryToken(const ustring& key, const KwRange& range) const;

template std::vector<Id> PiBas<Id>::serverSearch(const EncInd& encInd, const QueryToken& queryToken) const;
template std::vector<SrciDb1Doc> PiBas<Id>::serverSearch(const EncInd& encInd, const QueryToken& queryToken) const;
