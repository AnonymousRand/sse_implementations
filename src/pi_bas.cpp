#include <unordered_set>

#include <openssl/rand.h>

#include "pi_bas.h"

////////////////////////////////////////////////////////////////////////////////
// PiBas
////////////////////////////////////////////////////////////////////////////////

template <IDbDocDeriv DbDoc, class DbKw>
void PiBas<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    // generate key

    this->key = genKey(secParam);

    // build index

    if (db.empty()) {
        this->isIndEmpty = true;
        this->encInd = EncInd {};
        return;
    }
    this->isIndEmpty = false;

    // generate (plaintext) index of keywords to documents/ids mapping and list of unique keywords
    std::unordered_map<Range<DbKw>, std::vector<DbDoc>> index;
    std::unordered_set<Range<DbKw>> uniqueDbKwRanges;
    for (DbEntry<DbDoc, DbKw> entry : db) {
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
        // the paper uses different notation for the key generation here vs. in `genQueryToken()`
        // (`Trpdr`), but I'm fairly sure they mean the same thing, otherwise it doesn't work
        std::pair<ustring, ustring> subkeys = this->genQueryToken(dbKwRange);
        
        unsigned int counter = 0;
        // for each id in DB(w)
        auto itIdOpsWithSameKw = index.find(dbKwRange);
        for (DbDoc dbDoc : itIdOpsWithSameKw->second) {
            // l <- F(K_1, c); d <- Enc(K_2, id); c++
            ustring label = prf(subkeys.first, toUstr(counter));
            ustring iv = genIv(IV_LEN);
            ustring data = encrypt(ENC_CIPHER, subkeys.second, dbDoc.encode(), iv);
            // add (l, d) to list L (in lex order); we add straight to dictionary since we have ordered maps in C++
            // also store IV in plain along with encrypted value
            this->encInd[label] = std::pair {data, iv};
            counter++;
        }
    }
}

template <IDbDocDeriv DbDoc, class DbKw>
std::vector<DbDoc> PiBas<DbDoc, DbKw>::search(const Range<DbKw>& query) const {
    std::vector<DbDoc> allResults;
    // naive range search: just individually query every point in range
    for (DbKw dbKw = query.first; dbKw <= query.second; dbKw++) {
        std::pair<ustring, ustring> queryToken = this->genQueryToken(Range<DbKw> {dbKw, dbKw});
        std::vector<DbDoc> results = this->searchInd(queryToken);
        allResults.insert(allResults.end(), results.begin(), results.end());
    }
    return allResults;
}

template <IDbDocDeriv DbDoc, class DbKw>
std::pair<ustring, ustring> PiBas<DbDoc, DbKw>::genQueryToken(const Range<DbKw>& query) const {
    ustring K = prf(this->key, query.toUstr());
    int subkeyLen = K.length() / 2;
    ustring subkey1 = K.substr(0, subkeyLen);
    ustring subkey2 = K.substr(subkeyLen, subkeyLen);
    return std::pair<ustring, ustring> {subkey1, subkey2};
}

template <IDbDocDeriv DbDoc, class DbKw>
std::vector<DbDoc> PiBas<DbDoc, DbKw>::searchInd(const std::pair<ustring, ustring>& queryToken) const {
    return this->searchIndBase(queryToken);
}

template<>
std::vector<IdOp> PiBas<IdOp, Kw>::searchInd(const std::pair<ustring, ustring>& queryToken) const {
    std::vector<IdOp> results = this->searchIndBase(queryToken);
    return removeDeletedIdOps(results);
}

template <IDbDocDeriv DbDoc, class DbKw>
std::vector<DbDoc> PiBas<DbDoc, DbKw>::searchIndBase(const std::pair<ustring, ustring>& queryToken) const {
    ustring subkey1 = queryToken.first;
    ustring subkey2 = queryToken.second;
    std::vector<DbDoc> results;
    int counter = 0;
    
    // for c = 0 until `Get` returns error
    while (true) {
        // d <- Get(D, F(K_1, c)); c++
        ustring label = prf(subkey1, toUstr(counter));
        auto it = this->encInd.find(label);
        if (it == this->encInd.end()) {
            break;
        }
        std::pair<ustring, ustring> encIndV = it->second;
        ustring data = encIndV.first;
        // id <- Dec(K_2, d)
        ustring iv = encIndV.second;
        ustring ptext = decrypt(ENC_CIPHER, subkey2, data, iv);
        DbDoc result = DbDoc::decode(ptext);
        results.push_back(result);
        counter++;
    }

    return results;
}

// PiBas
template class PiBas<Id, Kw>;
template class PiBas<IdOp, Kw>;

// Log-SRC-i index 1
template class PiBas<SrciDb1Doc<Kw>, Kw>;

// Log-SRC-i index 2
template class PiBas<Id, Id>;
template class PiBas<IdOp, Id>;

////////////////////////////////////////////////////////////////////////////////
// PiBas (Result-Hiding)
////////////////////////////////////////////////////////////////////////////////

template <IDbDocDeriv DbDoc, class DbKw>
void PiBasResHiding<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    // generate keys

    this->key1 = genKey(secParam);
    this->key2 = genKey(secParam);

    // build index

    // generate (plaintext) index of keywords to documents/ids mapping and list of unique keywords
    std::unordered_map<Range<DbKw>, std::vector<DbDoc>> index;
    std::unordered_set<Range<DbKw>> uniqueDbKwRanges;
    for (DbEntry<DbDoc, DbKw> entry : db) {
        DbDoc dbDoc = entry.first;
        Range<DbKw> dbKwRange = entry.second;

        if (index.count(dbKwRange) == 0) {
            index[dbKwRange] = std::vector {dbDoc};
        } else {
            index[dbKwRange].push_back(dbDoc);
        }
        uniqueDbKwRanges.insert(dbKwRange);
    }

    // for each w in W
    for (Range<DbKw> dbKwRange : uniqueDbKwRanges) {
        ustring prfOutput = this->genQueryToken(dbKwRange);
        
        unsigned int counter = 0;
        auto itIdOpsWithSameKw = index.find(dbKwRange);
        for (DbDoc dbDoc : itIdOpsWithSameKw->second) {
            ustring label = findHash(HASH_FUNC, HASH_OUTPUT_LEN, prfOutput + toUstr(counter));
            ustring iv = genIv(IV_LEN);
            ustring data = encrypt(ENC_CIPHER, this->key2, dbDoc.encode(), iv);
            this->encInd[label] = std::pair {data, iv};
            counter++;
        }
    }
}

template <IDbDocDeriv DbDoc, class DbKw>
std::vector<DbDoc> PiBasResHiding<DbDoc, DbKw>::search(const Range<DbKw>& query) const {
    std::vector<DbDoc> allResults;
    // naive range search: just individually query every point in range
    for (DbKw dbKw = query.first; dbKw <= query.second; dbKw++) {
        ustring queryToken = this->genQueryToken(Range<DbKw> {dbKw, dbKw});
        std::vector<DbDoc> results = this->searchInd(queryToken);
        allResults.insert(allResults.end(), results.begin(), results.end());
    }
    return allResults;
}

template <IDbDocDeriv DbDoc, class DbKw>
ustring PiBasResHiding<DbDoc, DbKw>::genQueryToken(const Range<DbKw>& query) const {
    return prf(this->key1, query.toUstr());
}

template <IDbDocDeriv DbDoc, class DbKw>
std::vector<DbDoc> PiBasResHiding<DbDoc, DbKw>::searchInd(const ustring& queryToken) const {
    return this->searchIndBase(queryToken);
}

template<>
std::vector<IdOp> PiBasResHiding<IdOp, Kw>::searchInd(const ustring& queryToken) const {
    std::vector<IdOp> results = this->searchIndBase(queryToken);
    return removeDeletedIdOps(results);
}

template <IDbDocDeriv DbDoc, class DbKw>
std::vector<DbDoc> PiBasResHiding<DbDoc, DbKw>::searchIndBase(const ustring& queryToken) const {
    std::vector<DbDoc> results;
    int counter = 0;
    
    while (true) {
        ustring label = findHash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken + toUstr(counter));
        auto it = this->encInd.find(label);
        if (it == this->encInd.end()) {
            break;
        }
        std::pair<ustring, ustring> encIndV = it->second;
        ustring data = encIndV.first;
        ustring iv = encIndV.second;
        // technically we decrypt in the client, but since there's no client-server distinction
        // in this implementation we'll just decrypt immediately to make the code cleaner
        ustring ptext = decrypt(ENC_CIPHER, this->key2, data, iv);
        DbDoc result = DbDoc::decode(ptext);
        results.push_back(result);
        counter++;
    }

    return results;
}

template class PiBasResHiding<Id, Kw>;
template class PiBasResHiding<IdOp, Kw>;
