#include <unordered_set>

#include <openssl/rand.h>

#include "pi_bas.h"
#include "util/openssl.h"

template <IDbDocDeriv DbDoc, class DbKw>
void PiBas<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    // generate key

    unsigned char* key = new unsigned char[secParam];
    int res = RAND_priv_bytes(key, secParam);
    if (res != 1) {
        handleOpenSslErrors();
    }
    this->key = toUstr(key, secParam);
    delete[] key;

    // build index

    // generate (plaintext) index of keywords to dbDocuments/ids mapping and list of unique keywords
    std::unordered_map<Range<DbKw>, std::vector<DbDoc>> index;
    std::unordered_set<Range<DbKw>> uniqueDbKwRanges;
    for (std::pair entry : db) {
        DbDoc dbDoc = entry.first;
        Range<DbKw> dbKwRange = entry.second;

        if (index.count(dbKwRange) == 0) {
            index[dbKwRange] = std::vector {dbDoc};
        } else {
            index[dbKwRange].push_back(dbDoc);
        }
        uniqueDbKwRanges.insert(dbKwRange); // `unordered_set` will not insert duplicate elements
    }

    // for each w in W
    for (Range<DbKw> dbKwRange : uniqueDbKwRanges) {
        // K_1 || K_2 <- F(K, w)
        ustring K = prf(this->key, dbKwRange.toUstr());
        int subkeyLen = K.length() / 2;
        ustring subkey1 = K.substr(0, subkeyLen);
        ustring subkey2 = K.substr(subkeyLen, subkeyLen);
        
        unsigned int counter = 0;
        // for each id in DB(w)
        auto itIdOpsWithSameKw = index.find(dbKwRange);
        for (DbDoc dbDoc : itIdOpsWithSameKw->second) {
            // l <- F(K_1, c); d <- Enc(K_2, id)
            ustring label = prf(subkey1, toUstr(counter));
            ustring iv = genIv();
            ustring data = aesEncrypt(EVP_aes_256_cbc(), subkey2, dbDoc.encode(), iv);
            counter++;
            // add (l, d) to list L (in lex order); we add straight to dictionary since we have ordered maps in C++
            // also store IV in plain along with encrypted value
            this->encInd[label] = std::pair {data, iv};
        }
    }
}

template <IDbDocDeriv DbDoc, class DbKw>
std::vector<DbDoc> PiBas<DbDoc, DbKw>::search(const Range<DbKw>& query) const {
    std::vector<DbDoc> allResults;
    // naive range search: just individually query every point in range
    for (DbKw dbKw = query.first; dbKw <= query.second; dbKw++) {
        QueryToken queryToken = this->genQueryToken(Range<DbKw> {dbKw, dbKw});
        std::vector<DbDoc> results = this->searchInd(queryToken);
        allResults.insert(allResults.end(), results.begin(), results.end());
    }
    return allResults;
}

template <IDbDocDeriv DbDoc, class DbKw>
QueryToken PiBas<DbDoc, DbKw>::genQueryToken(const Range<DbKw>& query) const {
    // the paper uses different notation for the key generation here vs. in `setup()`;
    // but I'm fairly sure they meant the same thing, otherwise it doesn't work
    ustring K = prf(this->key, query.toUstr());
    int subkeyLen = K.length() / 2;
    ustring subkey1 = K.substr(0, subkeyLen);
    ustring subkey2 = K.substr(subkeyLen, subkeyLen);
    return QueryToken {subkey1, subkey2};
}

template <IDbDocDeriv DbDoc, class DbKw>
std::vector<DbDoc> PiBas<DbDoc, DbKw>::searchInd(const QueryToken& queryToken) const {
    return this->searchIndBase(queryToken);
}

// todo if compiler doesnt optimize out returns; is constantly erasing from the vector really faster?
template<>
std::vector<IdOp> PiBas<IdOp, Kw>::searchInd(const QueryToken& queryToken) const {
    std::vector<IdOp> results = this->searchIndBase(queryToken);
    std::vector<IdOp> finalResults;
    std::unordered_set<Id> deleted;

    // find all deletion tuples
    for (IdOp result : results) {
        Id id = result.get().first;
        Op op = result.get().second;
        if (op == DELETE) {
            deleted.insert(id);
        }
    }
    // copy over vector without deleted docs
    for (IdOp result : results) {
        Id id = result.get().first;
        Op op = result.get().second;
        if (op == INSERT && deleted.count(id) == 0) {
            finalResults.push_back(result);
        }
    }

    return finalResults;
}

template <IDbDocDeriv DbDoc, class DbKw>
std::vector<DbDoc> PiBas<DbDoc, DbKw>::searchIndBase(const QueryToken& queryToken) const {
    ustring subkey1 = queryToken.first;
    ustring subkey2 = queryToken.second;
    std::vector<DbDoc> results;
    int counter = 0;
    
    // for c = 0 until `Get` returns error
    while (true) {
        // d <- Get(D, F(K_1, c)); c++
        ustring label = prf(subkey1, toUstr(counter));
        counter++;
        auto it = this->encInd.find(label);
        if (it == this->encInd.end()) {
            break;
        }
        std::pair<ustring, ustring> encIndV = it->second;
        ustring data = encIndV.first;

        // id <- Dec(K_2, d)
        ustring iv = encIndV.second;
        ustring ptext = aesDecrypt(EVP_aes_256_cbc(), subkey2, data, iv);
        DbDoc result = DbDoc::decode(ptext);
        results.push_back(result);
    }

    return results;
}

// main
template class PiBas<Id, Kw>;
template class PiBas<IdOp, Kw>;

// Log-SRC-i index 1
template class PiBas<SrciDb1Doc<Kw>, Kw>;

// Log-SRC-i index 2
template class PiBas<Id, Id>;
template class PiBas<IdOp, Id>;
