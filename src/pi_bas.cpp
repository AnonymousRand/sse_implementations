#include <unordered_set>

#include <openssl/rand.h>

#include "pi_bas.h"
#include "util/openssl.h"

////////////////////////////////////////////////////////////////////////////////
// API Functions
////////////////////////////////////////////////////////////////////////////////

template <typename DbDoc>
void PiBas<DbDoc>::setup(int secParam, const Db<DbDoc>& db) {
    this->key = this->genKey(secParam);
    this->encInd = this->buildIndex(this->key, db);
}

template <typename DbDoc>
std::vector<Id> PiBas<DbDoc>::search(const KwRange& query) {
    QueryToken queryToken = this->genQueryToken(this->key, query);
    return this->serverSearch(this->encInd, queryToken);
}

////////////////////////////////////////////////////////////////////////////////
// Non-API Functions
////////////////////////////////////////////////////////////////////////////////

template <typename DbDoc>
ustring PiBas<DbDoc>::genKey(int secParam) const {
    unsigned char* key = new unsigned char[secParam];
    int res = RAND_priv_bytes(key, secParam);
    if (res != 1) {
        handleOpenSslErrors();
    }
    ustring ustrKey = toUstr(key, secParam);
    delete[] key;
    return ustrKey;
}

template <typename DbDoc>
template <typename DbDoc2, typename DbKw>
EncInd PiBas<DbDoc>::buildIndex(const ustring& key, const Db<DbDoc2, DbKw>& db) const {
    // generate (plaintext) index of keywords to documents/ids mapping and list of unique keywords
    std::unordered_map<DbKw, std::vector<DbDoc2>> index;
    std::unordered_set<DbKw> uniqueKws;
    for (auto entry : db) {
        DbDoc2 dbDoc = std::get<0>(entry);
        DbKw dbKw = std::get<1>(entry);

        if (index.count(dbKw) == 0) {
            index[dbKw] = std::vector {dbDoc};
        } else {
            index[dbKw].push_back(dbDoc);
        }
        uniqueKws.insert(dbKw); // `unordered_set` will not insert duplicate elements
    }

    EncInd encInd;
    // for each w in W
    for (DbKw dbKw : uniqueKws) {
        // K_1 || K_2 <- F(K, w)
        ustring K = prf(key, toUstr(dbKw));
        int subkeyLen = K.length() / 2;
        ustring subkey1 = K.substr(0, subkeyLen);
        ustring subkey2 = K.substr(subkeyLen, subkeyLen);
        
        unsigned int counter = 0;
        // for each id in DB(w)
        auto itDocsWithSameKw = index.find(dbKw);
        for (DbDoc2 dbDoc : itDocsWithSameKw->second) {
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

template <typename DbDoc>
template <typename RangeType>
QueryToken PiBas<DbDoc>::genQueryToken(const ustring& key, const Range<RangeType>& range) const {
    // the paper uses different notation for the key generation here vs. in `setup()`;
    // but I'm fairly sure they meant the same thing, otherwise it doesn't work
    ustring K = prf(key, toUstr(range));
    int subkeyLen = K.length() / 2;
    ustring subkey1 = K.substr(0, subkeyLen);
    ustring subkey2 = K.substr(subkeyLen, subkeyLen);
    return QueryToken {subkey1, subkey2};
}

template <typename DbDoc>
template <typename DbDoc2>
std::vector<DbDoc2> PiBas<DbDoc>::serverSearch(const EncInd& encInd, const QueryToken& queryToken) const {
    ustring subkey1 = queryToken.first;
    ustring subkey2 = queryToken.second;
    std::vector<DbDoc2> results;
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
        results.push_back(DbDoc2::decode(ptext));

        counter++;
    }

    return results;
}

////////////////////////////////////////////////////////////////////////////////
// Template Instantiations
////////////////////////////////////////////////////////////////////////////////

template class PiBas<Id>;
template class PiBas<IdOp>;

template EncInd PiBas<Id>::buildIndex(const ustring& key, const Db<Id>& db) const;
template EncInd PiBas<IdOp>::buildIndex(const ustring& key, const Db<IdOp>& db) const;
template EncInd PiBas<Id>::buildIndex(const ustring& key, const Db<SrciDb1Doc, KwRange>& db) const;
template EncInd PiBas<IdOp>::buildIndex(const ustring& key, const Db<SrciDb1Doc, KwRange>& db) const;
template EncInd PiBas<Id>::buildIndex(const ustring& key, const Db<Id, IdRange>& db) const;
template EncInd PiBas<IdOp>::buildIndex(const ustring& key, const Db<IdOp, IdOpRange>& db) const;

template QueryToken PiBas<Id>::genQueryToken(const ustring& key, const IdRange& range) const;
template QueryToken PiBas<IdOp>::genQueryToken(const ustring& key, const IdRange& range) const;
template QueryToken PiBas<Id>::genQueryToken(const ustring& key, const KwRange& range) const;
template QueryToken PiBas<IdOp>::genQueryToken(const ustring& key, const KwRange& range) const;

template std::vector<Id> PiBas<Id>::serverSearch(const EncInd& encInd, const QueryToken& queryToken) const;
template std::vector<IdOp> PiBas<IdOp>::serverSearch(const EncInd& encInd, const QueryToken& queryToken) const;
template std::vector<SrciDb1Doc> PiBas<Id>::serverSearch(const EncInd& encInd, const QueryToken& queryToken) const;
template std::vector<SrciDb1Doc> PiBas<IdOp>::serverSearch(const EncInd& encInd, const QueryToken& queryToken) const;
