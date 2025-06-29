#include "pi_bas_res_hiding.h"
#include "util/cryptography.h"


/******************************************************************************/
/* `PiBasResHidingBase`                                                       */
/******************************************************************************/


template <IDbDoc_ DbDoc, class DbKw>
PiBasResHidingBase<DbDoc, DbKw>::PiBasResHidingBase(EncIndType encIndType) {
    this->setEncIndType(encIndType);
}


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
        
        std::vector<DbDoc> dbDocsWithSameKw = ind.find(dbKwRange)->second;
        for (ulong counter = 0; counter < dbDocsWithSameKw.size(); counter++) {
            DbDoc dbDoc = dbDocsWithSameKw[counter];
            ustring label = findHash(HASH_FUNC, HASH_OUTPUT_LEN, prfOutput + toUstr(counter));
            ustring iv = genIv(IV_LEN);
            ustring encryptedDoc = padAndEncrypt(ENC_CIPHER, this->keyEnc, dbDoc.toUstr(), iv, ENC_IND_DOC_LEN - 1);
            this->encInd->write(label, std::pair {encryptedDoc, iv});
        }
    }
}


template <IDbDoc_ DbDoc, class DbKw>
std::vector<DbDoc> PiBasResHidingBase<DbDoc, DbKw>::searchGeneric(const Range<DbKw>& query) const {
    std::vector<DbDoc> allResults;
    for (DbKw dbKw = query.first; dbKw <= query.second; dbKw++) {
        std::vector<DbDoc> results = this->searchAsRangeUnderlyGeneric(Range {dbKw, dbKw});
        allResults.insert(allResults.end(), results.begin(), results.end());
    }
    return allResults;
}


template <IDbDoc_ DbDoc, class DbKw>
std::vector<DbDoc> PiBasResHidingBase<DbDoc, DbKw>::searchAsRangeUnderlyGeneric(const Range<DbKw>& query) const {
    std::vector<DbDoc> results;
    ustring queryToken = this->genQueryToken(query);
        
    ulong counter = 0;
    while (true) {
        ustring label = findHash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken + toUstr(counter));
        std::pair<ustring, ustring> encIndVal;
        int status = this->encInd->find(label, encIndVal);
        if (status == -1) {
            break;
        }
        ustring encryptedDoc = encIndVal.first;
        ustring iv = encIndVal.second;
        // technically we decrypt in the client, but since there's no client-server distinction in this implementation
        // we'll just decrypt immediately to make the code cleaner
        ustring decryptedDoc = decryptAndUnpad(ENC_CIPHER, this->keyEnc, encryptedDoc, iv);
        DbDoc result = DbDoc::fromUstr(decryptedDoc);
        results.push_back(result);
        counter++;
    }

    return results;
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


/******************************************************************************/
/* `PiBasResHiding`                                                           */
/******************************************************************************/


template <IDbDoc_ DbDoc, class DbKw>
PiBasResHiding<DbDoc, DbKw>::PiBasResHiding(EncIndType encIndType) : PiBasResHidingBase<DbDoc, DbKw>(encIndType) {}


template <IDbDoc_ DbDoc, class DbKw>
std::vector<DbDoc> PiBasResHiding<DbDoc, DbKw>::search(const Range<DbKw>& query) const {
    return this->searchGeneric(query);
}


template <IDbDoc_ DbDoc, class DbKw>
std::vector<DbDoc> PiBasResHiding<DbDoc, DbKw>::searchAsRangeUnderly(const Range<DbKw>& query) const {
    return this->searchAsRangeUnderlyGeneric(query);
}


template class PiBasResHiding<Doc, Kw>;       
template class PiBasResHiding<SrcIDb1Doc, Kw>;
//template class PiBasResHiding<Doc, IdAlias>;


/******************************************************************************/
/* `PiBasResHiding` Template Specialization                                   */
/******************************************************************************/


PiBasResHiding<Doc, Kw>::PiBasResHiding(EncIndType encIndType) : PiBasResHidingBase<Doc, Kw>(encIndType) {}


std::vector<Doc> PiBasResHiding<Doc, Kw>::search(const Range<Kw>& query) const {
    std::vector<Doc> results = this->searchGeneric(query);
    return removeDeletedDocs(results);
}


std::vector<Doc> PiBasResHiding<Doc, Kw>::searchAsRangeUnderly(const Range<Kw>& query) const {
    std::vector<Doc> results = this->searchAsRangeUnderlyGeneric(query);
    return removeDeletedDocs(results);
}
