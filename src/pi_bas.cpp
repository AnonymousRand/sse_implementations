#include "pi_bas.h"
#include "util/cryptography.h"


/******************************************************************************/
/* `PiBas`                                                                    */
/******************************************************************************/


template <IDbDoc_ DbDoc, class DbKw>
PiBasBase<DbDoc, DbKw>::PiBasBase(EncIndType encIndType) {
    this->setEncIndType(encIndType);
}


template <IDbDoc_ DbDoc, class DbKw>
PiBas<DbDoc, DbKw>::PiBas(EncIndType encIndType) : PiBasBase<DbDoc, DbKw>(encIndType) {}


PiBas<Doc, Kw>::PiBas(EncIndType encIndType) : PiBasBase<Doc, Kw>(encIndType) {}


template <IDbDoc_ DbDoc, class DbKw>
PiBasBase<DbDoc, DbKw>::~PiBasBase() {
    if (this->encInd != nullptr) {
        delete this->encInd;
        this->encInd = nullptr;
    }
}


template <IDbDoc_ DbDoc, class DbKw>
void PiBasBase<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {

    ////////////////////////////// generate keys ///////////////////////////////

    this->secParam = secParam;
    this->key = genKey(this->secParam);

    /////////////////////////////// build index ////////////////////////////////

    this->db = db;
    this->_isEmpty = db.empty();
    if (db.empty()) {
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

    this->encInd->clear();
    this->encInd->init(db.size());
    std::unordered_set<Range<DbKw>> uniqDbKwRanges = getUniqDbKwRanges(db);
    // for each w in W
    for (Range<DbKw> dbKwRange : uniqDbKwRanges) {
        // K_1 || K_2 <- F(K, w)
        // the paper uses different notation for the key generation here vs. in `genQueryToken()`
        // (`Trpdr`), but I'm fairly sure they mean the same thing, otherwise it doesn't work
        std::pair<ustring, ustring> subkeys = this->genQueryToken(dbKwRange);
        
        unsigned int counter = 0;
        // for each id in DB(w)
        auto itDocsWithSameKw = ind.find(dbKwRange);
        for (DbDoc dbDoc : itDocsWithSameKw->second) {
            // l <- F(K_1, c); d <- Enc(K_2, id); c++
            ustring label = prf(subkeys.first, toUstr(counter));
            ustring iv = genIv(IV_LEN);
            // for some reason padding to exactly n blocks generates n + 1 blocks, so we pad to one less byte
            ustring encryptedDoc = padAndEncrypt(ENC_CIPHER, subkeys.second, dbDoc.toUstr(), iv, ENC_IND_DOC_LEN - 1);
            // add (l, d) to list L (in lex order); we don't need to sort ourselves since C++ has ordered maps
            // also store IV in plain along with encrypted value
            this->encInd->write(label, std::pair {encryptedDoc, iv});
            counter++;
        }
    }
    this->encInd->flushWrite();
}


template <IDbDoc_ DbDoc, class DbKw>
std::vector<DbDoc> PiBas<DbDoc, DbKw>::search(const Range<DbKw>& query) const {
    return this->searchWithoutRemovingDels(query);
}


std::vector<Doc> PiBas<Doc, Kw>::search(const Range<Kw>& query) const {
    std::vector<Doc> results = this->searchWithoutRemovingDels(query);
    return removeDeletedDocs(results);
}


template <IDbDoc_ DbDoc, class DbKw>
std::vector<DbDoc> PiBasBase<DbDoc, DbKw>::searchWithoutRemovingDels(const Range<DbKw>& query) const {
    std::vector<DbDoc> allResults;

    // naive range search: just individually query every point in range
    for (DbKw dbKw = query.first; dbKw <= query.second; dbKw++) {
        std::pair<ustring, ustring> queryToken = this->genQueryToken(Range<DbKw> {dbKw, dbKw});
        ustring subkeyPrf = queryToken.first;
        ustring subkeyEnc = queryToken.second;
        std::vector<DbDoc> results;
        int counter = 0;
        
        // for c = 0 until `Get` returns error
        while (true) {
            // d <- Get(D, F(K_1, c)); c++
            ustring label = prf(subkeyPrf, toUstr(counter));
            std::pair<ustring, ustring> encIndVal;
            int status = this->encInd->find(label, encIndVal);
            if (status == -1) {
                break;
            }
            ustring encryptedDoc = encIndVal.first;
            // id <- Dec(K_2, d)
            ustring iv = encIndVal.second;
            ustring decryptedDoc = decryptAndUnpad(ENC_CIPHER, subkeyEnc, encryptedDoc, iv);
            DbDoc result = DbDoc::fromUstr(decryptedDoc);
            results.push_back(result);
            counter++;
        }

        allResults.insert(allResults.end(), results.begin(), results.end());
    }

    return allResults;
}


template <IDbDoc_ DbDoc, class DbKw>
std::pair<ustring, ustring> PiBasBase<DbDoc, DbKw>::genQueryToken(const Range<DbKw>& query) const {
    ustring K = prf(this->key, query.toUstr());
    int subkeyLen = K.length() / 2;
    ustring subkeyPrf = K.substr(0, subkeyLen);
    ustring subkeyEnc = K.substr(subkeyLen, subkeyLen);
    return std::pair {subkeyPrf, subkeyEnc};
}


template <IDbDoc_ DbDoc, class DbKw>
void PiBasBase<DbDoc, DbKw>::clear() {
    if (this->encInd != nullptr) {
        this->encInd->clear();
    }
    this->db.clear();
    this->_isEmpty = true;
}


template <IDbDoc_ DbDoc, class DbKw>
Db<DbDoc, DbKw> PiBasBase<DbDoc, DbKw>::getDb() const {
    return this->db;
}


template <IDbDoc_ DbDoc, class DbKw>
bool PiBasBase<DbDoc, DbKw>::isEmpty() const {
    return this->_isEmpty;
}


template <IDbDoc_ DbDoc, class DbKw>
void PiBasBase<DbDoc, DbKw>::setEncIndType(EncIndType encIndType) {
    switch (encIndType) {
        case EncIndType::RAM:
            this->encInd = new EncIndRam();
            break;
        case EncIndType::DISK:
            this->encInd = new EncIndDisk();
            break;
    }
}


template class PiBas<Doc, Kw>;        // PiBas
template class PiBas<SrcIDb1Doc, Kw>; // Log-SRC-i index 1
template class PiBas<Doc, IdAlias>;   // Log-SRC-i index 2


/******************************************************************************/
/* `PiBas` (Result-Hiding)                                                    */
/******************************************************************************/


template <IDbDoc_ DbDoc, class DbKw>
PiBasResHidingBase<DbDoc, DbKw>::PiBasResHidingBase(EncIndType encIndType) {
    this->setEncIndType(encIndType);
}


template <IDbDoc_ DbDoc, class DbKw>
PiBasResHiding<DbDoc, DbKw>::PiBasResHiding(EncIndType encIndType) : PiBasResHidingBase<DbDoc, DbKw>(encIndType) {}


PiBasResHiding<Doc, Kw>::PiBasResHiding(EncIndType encIndType) : PiBasResHidingBase<Doc, Kw>(encIndType) {}


template <IDbDoc_ DbDoc, class DbKw>
PiBasResHidingBase<DbDoc, DbKw>::~PiBasResHidingBase() {
    if (this->encInd != nullptr) {
        delete this->encInd;
        this->encInd = nullptr;
    }
}


template <IDbDoc_ DbDoc, class DbKw>
void PiBasResHidingBase<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {

    ////////////////////////////// generate keys ///////////////////////////////

    this->keyPrf = genKey(secParam);
    this->keyEnc = genKey(secParam);

    /////////////////////////////// build index ////////////////////////////////

    this->db = db;
    this->_isEmpty = db.empty();
    if (db.empty()) {
        this->encInd->clear();
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

    this->encInd->clear();
    this->encInd->init(db.size());
    std::unordered_set<Range<DbKw>> uniqDbKwRanges = getUniqDbKwRanges(db);
    // for each w in W
    for (Range<DbKw> dbKwRange : uniqDbKwRanges) {
        ustring prfOutput = this->genQueryToken(dbKwRange);
        
        unsigned int counter = 0;
        auto itDocsWithSameKw = ind.find(dbKwRange);
        for (DbDoc dbDoc : itDocsWithSameKw->second) {
            ustring label = findHash(HASH_FUNC, HASH_OUTPUT_LEN, prfOutput + toUstr(counter));
            ustring iv = genIv(IV_LEN);
            ustring encryptedDoc = padAndEncrypt(ENC_CIPHER, this->keyEnc, dbDoc.toUstr(), iv, ENC_IND_DOC_LEN - 1);
            this->encInd->write(label, std::pair {encryptedDoc, iv});
            counter++;
        }
    }
    this->encInd->flushWrite();
}


template <IDbDoc_ DbDoc, class DbKw>
std::vector<DbDoc> PiBasResHiding<DbDoc, DbKw>::search(const Range<DbKw>& query) const {
    return this->searchWithoutRemovingDels(query);
}


std::vector<Doc> PiBasResHiding<Doc, Kw>::search(const Range<Kw>& query) const {
    std::vector<Doc> results = this->searchWithoutRemovingDels(query);
    return removeDeletedDocs(results);
}


template <IDbDoc_ DbDoc, class DbKw>
std::vector<DbDoc> PiBasResHidingBase<DbDoc, DbKw>::searchWithoutRemovingDels(const Range<DbKw>& query) const {
    std::vector<DbDoc> allResults;
    for (DbKw dbKw = query.first; dbKw <= query.second; dbKw++) {
        ustring queryToken = this->genQueryToken(Range<DbKw> {dbKw, dbKw});
        std::vector<DbDoc> results;
        int counter = 0;
        
        while (true) {
            ustring label = findHash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken + toUstr(counter));
            std::pair<ustring, ustring> encIndVal;
            int status = this->encInd->find(label, encIndVal);
            if (status == -1) {
                break;
            }
            ustring encryptedDoc = encIndVal.first;
            ustring iv = encIndVal.second;
            // technically we decrypt in the client, but since there's no client-server distinction
            // in this implementation we'll just decrypt immediately to make the code cleaner
            ustring decryptedDoc = decryptAndUnpad(ENC_CIPHER, this->keyEnc, encryptedDoc, iv);
            DbDoc result = DbDoc::fromUstr(decryptedDoc);
            results.push_back(result);
            counter++;
        }

        allResults.insert(allResults.end(), results.begin(), results.end());
    }

    return allResults;
}


template <IDbDoc_ DbDoc, class DbKw>
ustring PiBasResHidingBase<DbDoc, DbKw>::genQueryToken(const Range<DbKw>& query) const {
    return prf(this->keyPrf, query.toUstr());
}


template <IDbDoc_ DbDoc, class DbKw>
void PiBasResHidingBase<DbDoc, DbKw>::clear() {
    if (this->encInd != nullptr) {
        this->encInd->clear();
    }
    this->db.clear();
    this->_isEmpty = true;
}


template <IDbDoc_ DbDoc, class DbKw>
Db<DbDoc, DbKw> PiBasResHidingBase<DbDoc, DbKw>::getDb() const {
    return this->db;
}


template <IDbDoc_ DbDoc, class DbKw>
bool PiBasResHidingBase<DbDoc, DbKw>::isEmpty() const {
    return this->_isEmpty;
}


template <IDbDoc_ DbDoc, class DbKw>
void PiBasResHidingBase<DbDoc, DbKw>::setEncIndType(EncIndType encIndType) {
    switch (encIndType) {
        case EncIndType::RAM:
            this->encInd = new EncIndRam();
            break;
        case EncIndType::DISK:
            this->encInd = new EncIndDisk();
            break;
    }
}


template class PiBasResHiding<Doc, Kw>;        // PiBas
template class PiBasResHiding<SrcIDb1Doc, Kw>; // Log-SRC-i index 1
//template class PiBasResHiding<Doc, IdAlias>;   // Log-SRC-i index 2
