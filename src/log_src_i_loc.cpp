#include "log_src_i_loc.h"
#include "util/cryptography.h"


/******************************************************************************/
/* `LogSrcILoc`                                                               */
/******************************************************************************/


template <template <class ...> class Underly> requires IsLogSrcILocUnderly<Underly<Doc, Kw>>
LogSrcILoc<Underly>::LogSrcILoc() : LogSrcIBase<Underly>() {}


template <template <class ...> class Underly> requires IsLogSrcILocUnderly<Underly<Doc, Kw>>
LogSrcILoc<Underly>::LogSrcILoc(EncIndType encIndType) : LogSrcIBase<Underly>(encIndType) {}


template <template <class ...> class Underly> requires IsLogSrcILocUnderly<Underly<Doc, Kw>>
void LogSrcILoc<Underly>::setup(int secParam, const Db<Doc, Kw>& db) {
    this->clear();

    this->db = db;
    this->_isEmpty = db.empty();

    ////////////////////////////// build index 2 ///////////////////////////////

    // sort documents by keyword to assign index 2 nodes/"identifier aliases"
    auto sortByKw = [](const DbEntry<Doc, Kw>& dbEntry1, const DbEntry<Doc, Kw>& dbEntry2) {
        return dbEntry1.first.getKw() < dbEntry2.first.getKw();
    };

    Db<Doc, Kw> dbSorted = db;
    std::sort(dbSorted.begin(), dbSorted.end(), sortByKw);
    Db<Doc, IdAlias> db2;
    std::unordered_map<Id, IdAlias> idAliasMapping; // for quick reference when buiding index 1
    for (long i = 0; i < dbSorted.size(); i++) {
        DbEntry<Doc, Kw> dbEntry = dbSorted[i];
        Doc doc = dbEntry.first;
        DbEntry<Doc, IdAlias> newDbEntry = DbEntry {doc, Range<IdAlias> {i, i}};
        db2.push_back(newDbEntry);
        Id id = doc.getId();
        idAliasMapping[id] = i;
    }

    // build TDAG 2 over id aliases
    IdAlias maxIdAlias = 0;
    for (DbEntry<Doc, IdAlias> dbEntry : db2) {
        IdAlias idAlias = dbEntry.second.first;
        if (idAlias > maxIdAlias) {
            maxIdAlias = idAlias;
        }
    }
    // pad TDAG 2 to power of two # of leaves, as that is required in the case of locality-aware Log-SRC-i*
    if (!std::has_single_bit(db2.size())) {
        long amountToPad = std::pow(2, std::ceil(std::log2(db2.size()))) - db2.size();
        for (long i = 0; i < amountToPad; i++) {
            maxIdAlias++;
            Doc dummyDoc {DUMMY, DUMMY, Op::DUMMY};
            DbEntry<Doc, IdAlias> dummyDbEntry = DbEntry {dummyDoc, Range<IdAlias> {maxIdAlias, maxIdAlias}};
            db2.push_back(dummyDbEntry);
        }
    }
    this->tdag2 = new TdagNode<IdAlias>(Range<IdAlias> {0, maxIdAlias});

    // replicate every document to all id alias ranges/TDAG 2 nodes that cover it
    long stop = db2.size();
    for (long i = 0; i < stop; i++) {
        DbEntry<Doc, IdAlias> dbEntry = db2[i];
        Doc doc = dbEntry.first;
        Range<IdAlias> idAliasRange = dbEntry.second;
        std::list<Range<IdAlias>> ancestors = this->tdag2->getLeafAncestors(idAliasRange);
        for (Range<IdAlias> ancestor : ancestors) {
            // ancestors include the leaf itself, which is already in `db2`
            if (ancestor == idAliasRange) {
                continue;
            }
            db2.push_back(std::pair {doc, ancestor});
        }
    }

    this->underly2->setup(secParam, db2);

    ////////////////////////////// build index 1 ///////////////////////////////

    // in the case of locality-aware Log-SRC-i*, we also need to clean up deleted docs and replace them and their
    // cancellation tuples with dummies so that we don't have more `Doc`s associated with some `Range<Kw>` than 
    // the size of this keyword range and hence the bucket size we've allocated for it in the encrypted index
    // (of course, we assume database search for ranges => no two `Doc`s share same `Kw` except for cancellation tuples)
    Range<Kw> kwBounds = findDbKwBounds(db);
    Db<Doc, Kw> dbCleaned;
    if (kwBounds.size() < db.size()) {
        std::unordered_set<Id> deletedIds;

        // find all cancellation tuples
        for (DbEntry<Doc, Kw> dbEntry : db) {
            Doc doc = dbEntry.first;
            Op op = doc.getOp();
            if (op == Op::DEL) {
                Id id = doc.getId();
                deletedIds.insert(id);
            }
        }
        // copy over vector without deleted (or dummy) docs
        for (DbEntry<Doc, Kw> dbEntry : db) {
            Doc doc = dbEntry.first;
            Id id = doc.getId();
            Op op = doc.getOp();
            if (op == Op::INS && deletedIds.count(id) == 0) {
                dbCleaned.push_back(dbEntry);
            }
        }
    } else {
        dbCleaned = db;
    }

    // assign id aliases/TDAG 2 nodes to documents based on index 2
    Db<SrcIDb1Doc, Kw> db1;
    for (DbEntry<Doc, Kw> dbEntry : dbCleaned) {
        Doc doc = dbEntry.first;
        Range<Kw> kwRange = dbEntry.second;
        IdAlias idAlias = idAliasMapping[doc.getId()];
        SrcIDb1Doc newDoc {kwRange, Range<IdAlias> {idAlias, idAlias}};
        DbEntry<SrcIDb1Doc, Kw> newDbEntry {newDoc, kwRange};
        db1.push_back(newDbEntry);
    }

    // build TDAG 1 over keywords
    kwBounds = findDbKwBounds(dbCleaned);
    // also pad to power of 2
    Kw maxKw = kwBounds.second;
    if (!std::has_single_bit((ulong)kwBounds.size())) {
        long amountToPad = std::pow(2, std::ceil(std::log2(kwBounds.size()))) - kwBounds.size();
        for (long i = 0; i < amountToPad; i++) {
            maxKw++;
            SrcIDb1Doc dummyDoc {DUMMY_RANGE<Kw>(), DUMMY_RANGE<IdAlias>()};
            DbEntry<SrcIDb1Doc, Kw> dummyDbEntry = DbEntry {dummyDoc, Range<IdAlias> {maxKw, maxKw}};
            db1.push_back(dummyDbEntry);
        }
    }
    this->tdag1 = new TdagNode<Kw>(Range {kwBounds.first, maxKw});

    // replicate every document (in this case `SrcIDb1Doc`s) to all keyword ranges/TDAG 1 nodes that cover it
    stop = db1.size();
    for (long i = 0; i < stop; i++) {
        DbEntry<SrcIDb1Doc, Kw> dbEntry = db1[i];
        SrcIDb1Doc doc = dbEntry.first;
        Range<Kw> kwRange = dbEntry.second;
        std::list<Range<Kw>> ancestors = this->tdag1->getLeafAncestors(kwRange);
        for (Range<Kw> ancestor : ancestors) {
            if (ancestor == kwRange) {
                continue;
            }
            db1.push_back(std::pair {doc, ancestor});
        }
    }

    this->underly1->setup(secParam, db1);
}


template class LogSrcIBase<PiBasLoc>;
template class LogSrcILoc<PiBasLoc>;


/******************************************************************************/
/* `PiBasLoc`                                                                 */
/******************************************************************************/


template <IDbDoc_ DbDoc, class DbKw>
PiBasLoc<DbDoc, DbKw>::PiBasLoc(EncIndType encIndType) {
    this->setEncIndType(encIndType);
}


template <IDbDoc_ DbDoc, class DbKw>
PiBasLoc<DbDoc, DbKw>::~PiBasLoc() {
    this->clear();
}


template <IDbDoc_ DbDoc, class DbKw>
void PiBasLoc<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
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

    this->encInd->init(db.size());
    std::unordered_set<Range<DbKw>> uniqDbKwRanges = getUniqDbKwRanges(db);
    Range<DbKw> dbKwBounds = findDbKwBounds(db);
    // note that `leafCount` isn't necessarily `db.size()`, since leaves must be contiguous but `db` might not be
    this->leafCount = dbKwBounds.size();
    this->minDbKw = dbKwBounds.first;
    for (Range<DbKw> dbKwRange : uniqDbKwRanges) {
        ustring prfOutput = this->genQueryToken(dbKwRange);
        
        auto iter = ind.find(dbKwRange);
        if (iter == ind.end()) {
            continue;
        }
        std::vector<DbDoc> dbDocsWithSameKw = iter->second;
        long dbKwResCount = dbDocsWithSameKw.size();
        this->kwResCounts[dbKwRange] = dbKwResCount;
        for (long counter = 0; counter < dbKwResCount; counter++) {
            DbDoc dbDoc = dbDocsWithSameKw[counter];
            ustring label = findHash(HASH_FUNC, HASH_OUTPUT_LEN, prfOutput + toUstr(counter));
            ustring iv = genIv(IV_LEN);
            ustring encryptedDbDoc = padAndEncrypt(ENC_CIPHER, this->keyEnc, dbDoc.toUstr(), iv, ENC_IND_DOC_LEN - 1);
            this->encInd->write(
                label, std::pair {encryptedDbDoc, iv}, dbKwRange, dbKwResCount, counter, this->minDbKw, this->leafCount
            );
        }
    }
}


template <IDbDoc_ DbDoc, class DbKw>
std::vector<DbDoc> PiBasLoc<DbDoc, DbKw>::searchBase(const Range<DbKw>& query) const {
    auto iter = this->kwResCounts.find(query);
    if (iter == this->kwResCounts.end()) {
        return std::vector<DbDoc> {};
    }
    long kwResCount = iter->second;

    std::vector<DbDoc> results;
    ustring queryToken = this->genQueryToken(query);
    for (long counter = 0; counter < kwResCount; counter++) {
        std::pair<ustring, ustring> encIndVal;
        this->encInd->find(query, kwResCount, counter, this->minDbKw, this->leafCount, encIndVal);
        ustring encryptedDbDoc = encIndVal.first;
        ustring iv = encIndVal.second;
        // technically we decrypt in the client, but since there's no client-server distinction in this implementation
        // we'll just decrypt immediately to make the code cleaner
        ustring decryptedDbDoc = decryptAndUnpad(ENC_CIPHER, this->keyEnc, encryptedDbDoc, iv);
        DbDoc result = DbDoc::fromUstr(decryptedDbDoc);
        results.push_back(result);
    }

    return results;
}


template <IDbDoc_ DbDoc, class DbKw>
void PiBasLoc<DbDoc, DbKw>::clear() {
    PiBasBase<DbDoc, DbKw>::clear();
    if (this->encInd != nullptr) {
        this->encInd->clear();
        this->encInd = nullptr;
    }
}


template <IDbDoc_ DbDoc, class DbKw>
void PiBasLoc<DbDoc, DbKw>::setEncIndType(EncIndType encIndType) {
    switch (encIndType) {
        case EncIndType::RAM:
            this->encInd = new EncIndLocRam<DbKw>();
            break;
        case EncIndType::DISK:
            this->encInd = new EncIndLocDisk<DbKw>();
            break;
    }
}


template class PiBasLoc<Doc, Kw>;       
template class PiBasLoc<SrcIDb1Doc, Kw>;
//template class PiBasLoc<Doc, IdAlias>;
