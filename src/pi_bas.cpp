#include <unordered_set>

#include <openssl/rand.h>

#include "pi_bas.h"
#include "util/openssl.h"

template <class DbDoc, class DbKw>
void PiBas<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->genKey(secParam);
    this->buildIndex(db);
}

template <class DbDoc, class DbKw>
std::vector<DbDoc> PiBas<DbDoc, DbKw>::search(const Range<DbKw>& query) const {
    std::vector<DbDoc> allResults;
    // naive range search: just individually query every point in range
    for (DbKw dbKw = query.first; dbKw <= query.second; dbKw++) {
        QueryToken queryToken = this->genQueryToken(Range<DbKw> {dbKw, dbKw});
        std::vector<DbDoc> results = this->genericSearch(queryToken); // todo have to do something about deletinos and detcing if its a Doc
        allResults.insert(allResults.end(), results.begin(), results.end());
    }
    return allResults;
}

template <class DbDoc, class DbKw>
void PiBas<DbDoc, DbKw>::genKey(int secParam) {
    unsigned char* key = new unsigned char[secParam];
    int res = RAND_priv_bytes(key, secParam);
    if (res != 1) {
        handleOpenSslErrors();
    }
    this->key = toUstr(key, secParam);
    delete[] key;
}

template <class DbDoc, class DbKw>
void PiBas<DbDoc, DbKw>::buildIndex(const Db<DbDoc, DbKw>& db) {
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
        auto itDocsWithSameKw = index.find(dbKwRange);
        for (DbDoc dbDoc : itDocsWithSameKw->second) {
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

template <class DbDoc, class DbKw>
QueryToken PiBas<DbDoc, DbKw>::genQueryToken(const Range<DbKw>& query) const {
    // the paper uses different notation for the key generation here vs. in `setup()`;
    // but I'm fairly sure they meant the same thing, otherwise it doesn't work
    ustring K = prf(this->key, query.toUstr());
    int subkeyLen = K.length() / 2;
    ustring subkey1 = K.substr(0, subkeyLen);
    ustring subkey2 = K.substr(subkeyLen, subkeyLen);
    return QueryToken {subkey1, subkey2};
}

template <class DbDoc, class DbKw>
std::vector<DbDoc> PiBas<DbDoc, DbKw>::genericSearch(const QueryToken& queryToken) const {
    ustring subkey1 = queryToken.first;
    ustring subkey2 = queryToken.second;
    std::vector<DbDoc> results;
    int counter = 0;
    
    // for c = 0 until `Get` returns error
    while (true) {
        // d <- Get(D, F(K_1, c))
        ustring label = prf(subkey1, toUstr(counter));
        auto it = this->encInd.find(label);
        if (it == this->encInd.end()) {
            break;
        }
        std::pair<ustring, ustring> encIndV = it->second;
        ustring data = encIndV.first;
        // id <- Dec(K_2, d)
        ustring iv = encIndV.second;
        ustring ptext = aesDecrypt(EVP_aes_256_cbc(), subkey2, data, iv);
        results.push_back(DbDoc::decode(ptext));

        counter++;
    }

    return results;
}

// todo if compiler doesnt optimize out returns; is constantly erasing from the vector really faster?
//template <class DbDoc, class DbKw>
//std::vector<Doc> PiBas<DbDoc, DbKw>::searchWithDels(const QueryToken& queryToken) const {
//    std::vector<Doc> results = this->genericSearch(queryToken);
//    std::vector<Doc> finalResults;
//    std::unordered_set<Id> deleted; // todo test is set is faster
//
//    // find deleted docs
//    for (Doc result : results) {
//        Id id = result.get().first;
//        Op op = result.get().second;
//        if (op == DELETE) {
//            deleted.insert(id);
//        }
//    }
//    // copy over vector without deleted docs
//    for (Doc result : results) {
//        Id id = result.get().first;
//        Op op = result.get().second;
//        if (op == INSERT && deleted.count(id) == 0) {
//            finalResults.push_back(result);
//        }
//    }
//    return finalResults;
//}

template class PiBas<Doc, Kw>;
template class PiBas<SrciDb1Doc<Kw>, Kw>;
template class PiBas<Doc, Id>;
