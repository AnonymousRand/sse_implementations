#include <openssl/rand.h>

#include "pi_bas.h"
#include "util/cryptography.h"

/******************************************************************************/
/* PiBas                                                                      */
/******************************************************************************/

template <IEncInd_ EncInd, IDbDoc_ DbDoc, class DbKw>
void PiBasBase<EncInd, DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    // generate key

    this->secParam = secParam;
    this->key = genKey(this->secParam);

    // build index

    this->db = db;
    this->_isEmpty = db.empty();
    if (db.empty()) {
        // clear memory
        this->encInd.clear();
        return;
    }

    // generate (plaintext) index of keywords to documents/ids mapping and list of unique keywords
    Ind<DbKw, DbDoc> ind;
    for (DbEntry<DbDoc, DbKw> entry : db) {
        DbDoc dbDoc = entry.first;
        Range<DbKw> dbKwRange = entry.second;

        if (ind.count(dbKwRange) == 0) {
            ind[dbKwRange] = std::vector {dbDoc};
        } else {
            ind[dbKwRange].push_back(dbDoc);
        }
    }
    // randomly permute documents associated with same keyword, required by some schemes on top of PiBas (e.g. Log-SRC)
    shuffleInd(ind);

    this->encInd.clear();
    this->encInd.init(db.size());
    std::unordered_set<Range<DbKw>> uniqDbKwRanges = getUniqDbKwRanges(db);
    // for each w in W
    for (Range<DbKw> dbKwRange : uniqDbKwRanges) {
        // K_1 || K_2 <- F(K, w)
        // the paper uses different notation for the key generation here vs. in `genQueryToken()`
        // (`Trpdr`), but I'm fairly sure they mean the same thing, otherwise it doesn't work
        std::pair<ustring, ustring> subkeys = this->genQueryToken(dbKwRange);
        
        unsigned int counter = 0;
        // for each id in DB(w)
        auto itIdOpsWithSameKw = ind.find(dbKwRange);
        for (DbDoc dbDoc : itIdOpsWithSameKw->second) {
            // l <- F(K_1, c); d <- Enc(K_2, id); c++
            ustring label = prf(subkeys.first, toUstr(counter));
            ustring iv = genIv(IV_LEN);
            // for some reason padding to exactly n blocks generates n + 1 blocks, so we pad to one less byte
            ustring encryptedDoc = padAndEncrypt(ENC_CIPHER, subkeys.second, dbDoc.encode(), iv, ENC_IND_DOC_LEN - 1);
            // add (l, d) to list L (in lex order); we don't need to sort ourselves since C++ has ordered maps
            // also store IV in plain along with encrypted value
            this->encInd.write(label, std::pair {encryptedDoc, iv});
            counter++;
        }
    }
    this->encInd.flushWrite();
}

template <IEncInd_ EncInd, IDbDoc_ DbDoc, class DbKw>
std::vector<DbDoc> PiBas<EncInd, DbDoc, DbKw>::search(const Range<DbKw>& query) const {
    return this->searchWithoutHandlingDels(query);
}

template <IEncInd_ EncInd, class DbKw>
std::vector<IdOp> PiBas<EncInd, IdOp, DbKw>::search(const Range<DbKw>& query) const {
    std::vector<IdOp> results = this->searchWithoutHandlingDels(query);
    return removeDeletedIdOps(results);
}

template <IEncInd_ EncInd, IDbDoc_ DbDoc, class DbKw>
std::vector<DbDoc> PiBasBase<EncInd, DbDoc, DbKw>::searchWithoutHandlingDels(const Range<DbKw>& query) const {
    std::vector<DbDoc> allResults;

    // naive range search: just individually query every point in range
    for (DbKw dbKw = query.first; dbKw <= query.second; dbKw++) {
        std::pair<ustring, ustring> queryToken = this->genQueryToken(Range<DbKw> {dbKw, dbKw});
        ustring subkey1 = queryToken.first;
        ustring subkey2 = queryToken.second;
        std::vector<DbDoc> results;
        int counter = 0;
        
        // for c = 0 until `Get` returns error
        while (true) {
            // d <- Get(D, F(K_1, c)); c++
            ustring label = prf(subkey1, toUstr(counter));
            std::pair<ustring, ustring> encIndV;
            int status = this->encInd.find(label, encIndV);
            if (status == -1) {
                break;
            }
            ustring encryptedDoc = encIndV.first;
            // id <- Dec(K_2, d)
            ustring iv = encIndV.second;
            ustring decryptedDoc = decryptAndUnpad(ENC_CIPHER, subkey2, encryptedDoc, iv);
            DbDoc result = DbDoc::decode(decryptedDoc);
            results.push_back(result);
            counter++;
        }

        allResults.insert(allResults.end(), results.begin(), results.end());
    }

    return allResults;
}

template <IEncInd_ EncInd, IDbDoc_ DbDoc, class DbKw>
std::pair<ustring, ustring> PiBasBase<EncInd, DbDoc, DbKw>::genQueryToken(const Range<DbKw>& query) const {
    ustring K = prf(this->key, query.toUstr());
    int subkeyLen = K.length() / 2;
    ustring subkey1 = K.substr(0, subkeyLen);
    ustring subkey2 = K.substr(subkeyLen, subkeyLen);
    return std::pair {subkey1, subkey2};
}

template <IEncInd_ EncInd, IDbDoc_ DbDoc, class DbKw>
Db<DbDoc, DbKw> PiBasBase<EncInd, DbDoc, DbKw>::getDb() const {
    return this->db;
}

template <IEncInd_ EncInd, IDbDoc_ DbDoc, class DbKw>
bool PiBasBase<EncInd, DbDoc, DbKw>::isEmpty() const {
    return this->_isEmpty;
}

// PiBas
template class PiBas<RamEncInd, Id, Kw>;
template class PiBas<RamEncInd, IdOp, Kw>;
template class PiBas<DiskEncInd, Id, Kw>;
template class PiBas<DiskEncInd, IdOp, Kw>;

// Log-SRC-i index 1
template class PiBas<RamEncInd, SrcIDb1Doc<Kw>, Kw>;
template class PiBas<DiskEncInd, SrcIDb1Doc<Kw>, Kw>;

// Log-SRC-i index 2
template class PiBas<RamEncInd, Id, IdAlias>;
template class PiBas<RamEncInd, IdOp, IdAlias>;
template class PiBas<DiskEncInd, Id, IdAlias>;
template class PiBas<DiskEncInd, IdOp, IdAlias>;

/******************************************************************************/
/* PiBas (Result-Hiding)                                                      */
/******************************************************************************/

template <IEncInd_ EncInd, IDbDoc_ DbDoc, class DbKw>
void PiBasResHidingBase<EncInd, DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    // generate keys

    this->key1 = genKey(secParam);
    this->key2 = genKey(secParam);

    // build index

    this->db = db;
    if (db.empty()) {
        this->_isEmpty = true;
        this->encInd = EncInd {};
        return;
    }
    this->_isEmpty = false;

    // generate (plaintext) index of keywords to documents/ids mapping and list of unique keywords
    Ind<DbKw, DbDoc> ind;
    for (DbEntry<DbDoc, DbKw> entry : db) {
        DbDoc dbDoc = entry.first;
        Range<DbKw> dbKwRange = entry.second;

        if (ind.count(dbKwRange) == 0) {
            ind[dbKwRange] = std::vector {dbDoc};
        } else {
            ind[dbKwRange].push_back(dbDoc);
        }
    }

    this->encInd.clear();
    this->encInd.init(db.size());
    std::unordered_set<Range<DbKw>> uniqDbKwRanges = getUniqDbKwRanges(db);
    // for each w in W
    for (Range<DbKw> dbKwRange : uniqDbKwRanges) {
        ustring prfOutput = this->genQueryToken(dbKwRange);
        
        unsigned int counter = 0;
        auto itIdOpsWithSameKw = ind.find(dbKwRange);
        for (DbDoc dbDoc : itIdOpsWithSameKw->second) {
            ustring label = findHash(HASH_FUNC, HASH_OUTPUT_LEN, prfOutput + toUstr(counter));
            ustring iv = genIv(IV_LEN);
            ustring encryptedDoc = padAndEncrypt(ENC_CIPHER, this->key2, dbDoc.encode(), iv, ENC_IND_DOC_LEN - 1);
            this->encInd.write(label, std::pair {encryptedDoc, iv});
            counter++;
        }
    }
    this->encInd.flushWrite();
}

template <IEncInd_ EncInd, IDbDoc_ DbDoc, class DbKw>
std::vector<DbDoc> PiBasResHiding<EncInd, DbDoc, DbKw>::search(const Range<DbKw>& query) const {
    return this->searchWithoutHandlingDels(query);
}

template <IEncInd_ EncInd, class DbKw>
std::vector<IdOp> PiBasResHiding<EncInd, IdOp, DbKw>::search(const Range<DbKw>& query) const {
    std::vector<IdOp> results = this->searchWithoutHandlingDels(query);
    return removeDeletedIdOps(results);
}

template <IEncInd_ EncInd, IDbDoc_ DbDoc, class DbKw>
std::vector<DbDoc> PiBasResHidingBase<EncInd, DbDoc, DbKw>::searchWithoutHandlingDels(const Range<DbKw>& query) const {
    std::vector<DbDoc> allResults;
    for (DbKw dbKw = query.first; dbKw <= query.second; dbKw++) {
        ustring queryToken = this->genQueryToken(Range<DbKw> {dbKw, dbKw});
        std::vector<DbDoc> results;
        int counter = 0;
        
        while (true) {
            ustring label = findHash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken + toUstr(counter));
            std::pair<ustring, ustring> encIndV;
            int status = this->encInd.find(label, encIndV);
            if (status == -1) {
                break;
            }
            ustring encryptedDoc = encIndV.first;
            ustring iv = encIndV.second;
            // technically we decrypt in the client, but since there's no client-server distinction
            // in this implementation we'll just decrypt immediately to make the code cleaner
            ustring decryptedDoc = decryptAndUnpad(ENC_CIPHER, this->key2, encryptedDoc, iv);
            DbDoc result = DbDoc::decode(decryptedDoc);
            results.push_back(result);
            counter++;
        }

        allResults.insert(allResults.end(), results.begin(), results.end());
    }

    return allResults;
}

template <IEncInd_ EncInd, IDbDoc_ DbDoc, class DbKw>
ustring PiBasResHidingBase<EncInd, DbDoc, DbKw>::genQueryToken(const Range<DbKw>& query) const {
    return prf(this->key1, query.toUstr());
}

template <IEncInd_ EncInd, IDbDoc_ DbDoc, class DbKw>
Db<DbDoc, DbKw> PiBasResHidingBase<EncInd, DbDoc, DbKw>::getDb() const {
    return this->db;
}

template <IEncInd_ EncInd, IDbDoc_ DbDoc, class DbKw>
bool PiBasResHidingBase<EncInd, DbDoc, DbKw>::isEmpty() const {
    return this->_isEmpty;
}

// PiBas
template class PiBasResHiding<RamEncInd, Id, Kw>;
template class PiBasResHiding<RamEncInd, IdOp, Kw>;
template class PiBasResHiding<DiskEncInd, Id, Kw>;
template class PiBasResHiding<DiskEncInd, IdOp, Kw>;

// Log-SRC-i index 1
template class PiBasResHiding<RamEncInd, SrcIDb1Doc<Kw>, Kw>;
template class PiBasResHiding<DiskEncInd, SrcIDb1Doc<Kw>, Kw>;

// Log-SRC-i index 2
template class PiBasResHiding<RamEncInd, Id, IdAlias>;
template class PiBasResHiding<RamEncInd, IdOp, IdAlias>;
template class PiBasResHiding<DiskEncInd, Id, IdAlias>;
template class PiBasResHiding<DiskEncInd, IdOp, IdAlias>;
