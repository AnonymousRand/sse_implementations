#include "pi_bas.h"
#include "util/cryptography.h"


/******************************************************************************/
/* `PiBasBase`                                                                */
/******************************************************************************/


template <IDbDoc_ DbDoc, class DbKw>
std::vector<DbDoc> PiBasBase<DbDoc, DbKw>::search(
    const Range<DbKw>& query, bool shouldCleanUpResults, bool isNaive
) const {
    std::vector<DbDoc> allResults;

    if (isNaive) {
        // naive range search: just individually query every point in range
        for (DbKw dbKw = query.first; dbKw <= query.second; dbKw++) {
            std::vector<DbDoc> results = this->searchBase(Range {dbKw, dbKw});
            allResults.insert(allResults.end(), results.begin(), results.end());
        }
    } else {
        // search entire range in one go (i.e. `query` itself must be in the db), e.g. as underlying for Log-SRC
        allResults = this->searchBase(query);
    }

    if (shouldCleanUpResults) {
        cleanUpResults(allResults);
    }

    return allResults;
}


template <IDbDoc_ DbDoc, class DbKw>
ustring PiBasBase<DbDoc, DbKw>::genQueryToken(const Range<DbKw>& query) const {
    return prf(this->keyPrf, query.toUstr());
}


template <IDbDoc_ DbDoc, class DbKw>
void PiBasBase<DbDoc, DbKw>::clear() {
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


/******************************************************************************/
/* `PiBas`                                                                    */
/******************************************************************************/


template <IDbDoc_ DbDoc, class DbKw>
PiBas<DbDoc, DbKw>::PiBas(EncIndType encIndType) {
    this->setEncIndType(encIndType);
}


template <IDbDoc_ DbDoc, class DbKw>
PiBas<DbDoc, DbKw>::~PiBas() {
    this->clear();
}


template <IDbDoc_ DbDoc, class DbKw>
void PiBas<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->clear();

    ////////////////////////////// generate keys ///////////////////////////////

    this->keyPrf = genKey(secParam);
    this->keyEnc = genKey(secParam);

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

    this->encInd->init(db.size());
    std::unordered_set<Range<DbKw>> uniqDbKwRanges = getUniqDbKwRanges(db);
    // for each w in W
    for (Range<DbKw> dbKwRange : uniqDbKwRanges) {
        // this is PRF(K_1, w)
        ustring queryToken = this->genQueryToken(dbKwRange);

        auto iter = ind.find(dbKwRange);
        if (iter == ind.end()) {
            continue;
        }
        std::vector<DbDoc> dbDocsWithSameKw = iter->second;
        // for each id in DB(w)
        for (long counter = 0; counter < dbDocsWithSameKw.size(); counter++) {
            DbDoc dbDoc = dbDocsWithSameKw[counter];
            // l <- Hash(PRF(K_1, w) || c)
            ustring label = findHash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken + toUstr(counter));
            // d <- Enc(K_2, w, id)
            ustring iv = genIv(IV_LEN);
            // for some reason padding to exactly n blocks generates n + 1 blocks, so we pad to one less byte
            ustring encryptedDbDoc = padAndEncrypt(ENC_CIPHER, this->keyEnc, dbDoc.toUstr(), iv, ENC_IND_DOC_LEN - 1);
            // store (l, d) into key-value store
            // also store IV in plain along with encrypted value
            this->encInd->write(label, std::pair {encryptedDbDoc, iv});
        }
    }
}


template <IDbDoc_ DbDoc, class DbKw>
std::vector<DbDoc> PiBas<DbDoc, DbKw>::searchBase(const Range<DbKw>& query) const {
    std::vector<DbDoc> results;
    ustring queryToken = this->genQueryToken(query);
        
    // for c = 0 until `Get` returns error
    long counter = 0;
    while (true) {
        // l <- Hash(PRF(K_1, w) || c) (same as in `setup()`!)
        ustring label = findHash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken + toUstr(counter));
        // res <- encInd.get(l)
        std::pair<ustring, ustring> encIndVal;
        int status = this->encInd->find(label, encIndVal);
        if (status == -1) {
            break;
        }
        ustring encryptedDbDoc = encIndVal.first;
        ustring iv = encIndVal.second;
        // technically we decrypt in the client, but since there's no client-server distinction in this implementation
        // we'll just decrypt immediately to make the code cleaner
        ustring decryptedDbDoc = decryptAndUnpad(ENC_CIPHER, this->keyEnc, encryptedDbDoc, iv);
        DbDoc result = DbDoc::fromUstr(decryptedDbDoc);
        results.push_back(result);
        counter++;
    }

    return results;
}


template <IDbDoc_ DbDoc, class DbKw>
void PiBas<DbDoc, DbKw>::clear() {
    PiBasBase<DbDoc, DbKw>::clear();
    if (this->encInd != nullptr) {
        this->encInd->clear();
    }
}


template <IDbDoc_ DbDoc, class DbKw>
void PiBas<DbDoc, DbKw>::setEncIndType(EncIndType encIndType) {
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
//template class PiBas<Doc, IdAlias>; // Log-SRC-i index 2
