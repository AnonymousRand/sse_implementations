#include "log_src_i_loc.h"

#include "utils/cryptography.h"


/******************************************************************************/
/* `LogSrcILoc`                                                               */
/******************************************************************************/


template <template <class ...> class Underly> requires IsLogSrcILocUnderly<Underly<Doc, Kw>>
LogSrcILoc<Underly>::LogSrcILoc() : LogSrcIBase<Underly>() {}


template <template <class ...> class Underly> requires IsLogSrcILocUnderly<Underly<Doc, Kw>>
void LogSrcILoc<Underly>::setup(int secParam, const Db<Doc, Kw>& db) {
    this->clear();

    this->db = db;
    this->size = db.size();

    ////////////////////////////// build index 2 ///////////////////////////////

    // sort documents by keyword
    auto sortByKw = [](const DbEntry<Doc, Kw>& dbEntry1, const DbEntry<Doc, Kw>& dbEntry2) {
        return dbEntry1.first.getKw() < dbEntry2.first.getKw();
    };
    Db<Doc, Kw> dbSorted = db;
    std::sort(dbSorted.begin(), dbSorted.end(), sortByKw);

    // assign index 2 nodes/"identifier aliases" and populate both `db1` and `db2` leaves with this information
    Db<SrcIDb1Doc, Kw> db1;
    Db<Doc, IdAlias> db2;
    db1.reserve(dbSorted.size());
    db2.reserve(dbSorted.size());
    Kw prevKw = DUMMY;
    IdAlias firstIdAliasWithKw;
    IdAlias lastIdAliasWithKw;
    auto addDb1Leaf = [&db1](Kw prevKw, IdAlias firstIdAliasWithKw, IdAlias lastIdAliasWithKw) {
        Range<IdAlias> idAliasRangeWithKw {firstIdAliasWithKw, lastIdAliasWithKw};
        SrcIDb1Doc newDoc1 {prevKw, idAliasRangeWithKw};
        DbEntry<SrcIDb1Doc, Kw> newDbEntry1 {newDoc1, Range {prevKw, prevKw}};
        db1.push_back(newDbEntry1);
    };

    for (long idAlias = 0; idAlias < dbSorted.size(); idAlias++) {
        DbEntry<Doc, Kw> dbEntry = dbSorted[idAlias];
        Doc doc = dbEntry.first;
        Kw kw = dbEntry.second.first; // entries in `db` must have size 1 `Kw` ranges!
        // populate `db2` leaves
        DbEntry<Doc, IdAlias> newDbEntry2 = DbEntry {doc, Range<IdAlias> {idAlias, idAlias}};
        db2.push_back(newDbEntry2);

        // populate `db1` leaves
        if (kw != prevKw) {
            if (prevKw != DUMMY) {
                addDb1Leaf(prevKw, firstIdAliasWithKw, lastIdAliasWithKw);
            }
            prevKw = kw;
            firstIdAliasWithKw = idAlias;
            lastIdAliasWithKw = idAlias;
        } else {
            lastIdAliasWithKw = idAlias;
        }
    }
    // make sure to write in last `Kw` (which cannot be detected by `kw != prevKw` in the loop above)
    if (prevKw != DUMMY) {
        addDb1Leaf(prevKw, firstIdAliasWithKw, lastIdAliasWithKw);
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
        db2.reserve(db2.size() + amountToPad);
        for (long i = 0; i < amountToPad; i++) {
            maxIdAlias++;
            Doc dummyDoc {DUMMY, DUMMY, Op::DUMMY};
            DbEntry<Doc, IdAlias> dummyDbEntry = DbEntry {dummyDoc, Range<IdAlias> {maxIdAlias, maxIdAlias}};
            db2.push_back(dummyDbEntry);
        }
    }
    this->tdag2 = new TdagNode<IdAlias>(Range {IdAlias(0), maxIdAlias});

    // replicate every document to all id alias ranges/TDAG 2 nodes that cover it
    long stop = db2.size();
    long topLevel = std::log2(stop);
    long newSize = topLevel * (2 * stop) - (1 - std::pow(2, -topLevel)) * std::pow(2, topLevel+1) + stop;
    db2.reserve(newSize);
    for (long i = 0; i < stop; i++) {
        DbEntry<Doc, IdAlias> dbEntry = db2[i];
        Doc doc = dbEntry.first;
        Range<IdAlias> idAliasRange = dbEntry.second;
        std::list<Range<IdAlias>> ancestors = this->tdag2->getLeafAncestors(idAliasRange);
        for (Range<IdAlias> ancestor : ancestors) {
            if (ancestor == idAliasRange) {
                continue;
            }
            db2.push_back(std::pair {doc, ancestor});
        }
    }

    this->underly2->setup(secParam, db2);

    ////////////////////////////// build index 1 ///////////////////////////////

    // build TDAG 1 over `Kw`s
    // since `Kw`s have no guarantee of being contiguous but the leaves and hence bottom level in the index must be,
    // we need to pad `db1` to have (exactly) one doc per `Kw` (we can just leave blanks in the case of non-locality
    // Log-SRC-i since docs are placed pseudorandomly in the index, but here we have to pad to avoid empty buckets
    // in the index that the server knows corresponds to a lack of docs with that keyword)
    DbEntry<Doc, Kw> dbEntry = dbSorted[0];
    prevKw = dbEntry.second.first;
    SrcIDb1Doc dummySrcIDb1Doc {DUMMY, DUMMY_RANGE<IdAlias>()};
    for (long i = 1; i < dbSorted.size(); i++) {
        dbEntry = dbSorted[i];
        Kw kw = dbEntry.second.first;
        if (kw == prevKw) {
            continue;
        }
        // if non-contiguous `Kw`s detected, fill in the gap with dummies
        if (kw > prevKw + 1) {
            for (Kw paddingKw = prevKw + 1; paddingKw < kw; paddingKw++) {
                DbEntry<SrcIDb1Doc, Kw> dummyDbEntry = DbEntry {dummySrcIDb1Doc, Range<Kw> {paddingKw, paddingKw}};
                db1.push_back(dummyDbEntry);
            }
        }
        prevKw = kw;
    }

    // after guaranteeing contiguous-ness of `Kw`s, pad `db1` to power of 2 as well
    Range<Kw> db1KwBounds = findDbKwBounds(db1);
    Kw maxDb1Kw = db1KwBounds.second;
    if (!std::has_single_bit((ulong)db1.size())) {
        long amountToPad = std::pow(2, std::ceil(std::log2(db1.size()))) - db1.size();
        db1.reserve(db1.size() + amountToPad);
        for (long i = 0; i < amountToPad; i++) {
            maxDb1Kw++;
            DbEntry<SrcIDb1Doc, Kw> dummyDbEntry = DbEntry {dummySrcIDb1Doc, Range<Kw> {maxDb1Kw, maxDb1Kw}};
            db1.push_back(dummyDbEntry);
        }
    }
    this->tdag1 = new TdagNode<Kw>(Range {db1KwBounds.first, maxDb1Kw});

    // replicate every document (in this case `SrcIDb1Doc`s) to all keyword ranges/TDAG 1 nodes that cover it
    stop = db1.size();
    topLevel = std::log2(stop);
    newSize = topLevel * (2 * stop) - (1 - std::pow(2, -topLevel)) * std::pow(2, topLevel+1) + stop;
    db1.reserve(newSize);
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


template class LogSrcILoc<PiBasLoc>;


/******************************************************************************/
/* `PiBasLoc`                                                                 */
/******************************************************************************/


template <IsDbDOc DbDoc, class DbKw>
PiBasLoc<DbDoc, DbKw>::~PiBasLoc() {
    this->clear();
}


template <IsDbDOc DbDoc, class DbKw>
void PiBasLoc<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->clear();
    
    ////////////////////////////// generate keys ///////////////////////////////

    this->keyPrf = genKey(secParam);
    this->keyEnc = genKey(secParam);

    /////////////////////////////// build index ////////////////////////////////

    this->db = db;
    this->size = db.size();
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
        ustring queryToken = this->genQueryToken(dbKwRange);
        
        auto iter = ind.find(dbKwRange);
        if (iter == ind.end()) {
            continue;
        }
        std::vector<DbDoc> dbDocsWithSameKw = iter->second;
        long dbKwResCount = dbDocsWithSameKw.size();
        this->kwResCounts[dbKwRange] = dbKwResCount;
        for (long counter = 0; counter < dbKwResCount; counter++) {
            DbDoc dbDoc = dbDocsWithSameKw[counter];
            ustring label = findHash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken + toUstr(counter));
            ustring iv = genIv(IV_LEN);
            ustring encryptedDbDoc = padAndEncrypt(ENC_CIPHER, this->keyEnc, dbDoc.toUstr(), iv, ENC_IND_DOC_LEN - 1);
            this->encInd->write(
                label, std::pair {encryptedDbDoc, iv}, dbKwRange, dbKwResCount, counter, this->minDbKw, this->leafCount
            );
        }
    }
}


template <IsDbDOc DbDoc, class DbKw>
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


template <IsDbDOc DbDoc, class DbKw>
void PiBasLoc<DbDoc, DbKw>::clear() {
    PiBasBase<DbDoc, DbKw>::clear();
    if (this->encInd != nullptr) {
        this->encInd->clear();
    }
}


template class PiBasLoc<Doc, Kw>;       
template class PiBasLoc<SrcIDb1Doc, Kw>;
//template class PiBasLoc<Doc, IdAlias>;
