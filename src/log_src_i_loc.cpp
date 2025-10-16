#include "log_src_i_loc.h"

#include "utils/cryptography.h"


//==============================================================================
// `LogSrcILoc`
//==============================================================================


template <template <class ...> class Underly> requires IsLogSrcILocUnderly<Underly<Doc<>, Kw>>
LogSrcILoc<Underly>::LogSrcILoc() : LogSrcIBase<Underly>() {}


template <template <class ...> class Underly> requires IsLogSrcILocUnderly<Underly<Doc<>, Kw>>
void LogSrcILoc<Underly>::setup(int secParam, const Db<Doc<>, Kw>& db) {
    this->clear();

    this->secParam = secParam;
    this->size = db.size();
    this->origDbUnderly->setup(secParam, db);

    //--------------------------------------------------------------------------
    // build index 2

    // sort documents by keyword
    auto sortByKw = [](const DbEntry<Doc<>, Kw>& dbEntry1, const DbEntry<Doc<>, Kw>& dbEntry2) {
        return dbEntry1.first.getKw() < dbEntry2.first.getKw();
    };
    Db<Doc<>, Kw> dbSorted = db;
    std::sort(dbSorted.begin(), dbSorted.end(), sortByKw);

    // assign index 2 nodes/"identifier aliases" and populate both `db1` and `db2` leaves with this information
    Db<SrcIDb1Doc, Kw> db1;
    Db<Doc<IdAlias>, IdAlias> db2;
    db1.reserve(dbSorted.size());
    db2.reserve(dbSorted.size());
    Kw prevKw = DUMMY;
    IdAlias firstIdAliasWithKw;
    IdAlias lastIdAliasWithKw;
    auto addDb1Leaf = [&db1](Kw prevKw, IdAlias firstIdAliasWithKw, IdAlias lastIdAliasWithKw) {
        Range<IdAlias> idAliasRangeWithKw {firstIdAliasWithKw, lastIdAliasWithKw};
        Range<Kw> kwRange {prevKw, prevKw};
        SrcIDb1Doc newDoc {prevKw, idAliasRangeWithKw, kwRange};
        DbEntry<SrcIDb1Doc, Kw> newDbEntry {newDoc, kwRange};
        db1.push_back(newDbEntry);
    };
    for (long idAlias = 0; idAlias < dbSorted.size(); idAlias++) {
        DbEntry<Doc<>, Kw> dbEntry = dbSorted[idAlias];
        Doc<> doc = dbEntry.first;
        Kw kw = dbEntry.second.first; // entries in `db` must have size 1 `Kw` ranges!
        // populate `db2` leaves
        Range<IdAlias> idAliasRange {idAlias, idAlias};
        Doc<IdAlias> newDb2Doc(doc.get(), idAliasRange);
        DbEntry<Doc<IdAlias>, IdAlias> newDb2Entry = DbEntry {newDb2Doc, idAliasRange};
        db2.push_back(newDb2Entry);

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
    for (DbEntry<Doc<IdAlias>, IdAlias> dbEntry : db2) {
        IdAlias idAlias = dbEntry.second.first;
        if (idAlias > maxIdAlias) {
            maxIdAlias = idAlias;
        }
    }
    // pad TDAG 2 to a power of two number of leaves, as is required for Log-SRC-i*
    if (!std::has_single_bit(db2.size())) {
        long amountToPad = std::pow(2, std::ceil(std::log2(db2.size()))) - db2.size();
        db2.reserve(db2.size() + amountToPad);
        for (long i = 0; i < amountToPad; i++) {
            maxIdAlias++;
            Range<IdAlias> idAliasRange {maxIdAlias, maxIdAlias};
            Doc<IdAlias> dummyDoc {DUMMY, DUMMY, Op::DUMMY, idAliasRange};
            DbEntry<Doc<IdAlias>, IdAlias> dummyDbEntry = DbEntry {dummyDoc, idAliasRange};
            db2.push_back(dummyDbEntry);
        }
    }
    this->tdag2 = new TdagNode<IdAlias>(Range<IdAlias> {0, maxIdAlias});

    // replicate every document to all id alias ranges/TDAG 2 nodes that cover it
    long stop = db2.size();
    db2.reserve(calcTdagItemCount(stop));
    for (long i = 0; i < stop; i++) {
        DbEntry<Doc<IdAlias>, IdAlias> dbEntry = db2[i];
        Doc<IdAlias> doc = dbEntry.first;
        Range<IdAlias> idAliasRange = dbEntry.second;
        std::list<Range<IdAlias>> ancestors = this->tdag2->getLeafAncestors(idAliasRange);
        for (Range<IdAlias> ancestor : ancestors) {
            if (ancestor == idAliasRange) {
                continue;
            }
            Doc<IdAlias> newDoc(doc.get(), ancestor);
            db2.push_back(std::pair {newDoc, ancestor});
        }
    }

    this->underly2->setup(secParam, db2);

    //--------------------------------------------------------------------------
    // build index 1

    // build TDAG 1 over `Kw`s
    // since `Kw`s have no guarantee of being contiguous but the leaves and hence bottom level in the index must be,
    // we need to pad `db1` to have (exactly) one doc per `Kw` (we can just leave blanks in the case of non-locality
    // Log-SRC-i since docs are placed pseudorandomly in the index, but here we have to pad to avoid empty buckets
    // in the index that the server knows corresponds to a lack of docs with that keyword)
    DbEntry<Doc<>, Kw> dbEntry = dbSorted[0];
    prevKw = dbEntry.second.first;
    for (long i = 1; i < dbSorted.size(); i++) {
        dbEntry = dbSorted[i];
        Kw kw = dbEntry.second.first;
        // if non-contiguous `Kw`s detected, fill in the gap with dummies
        if (kw - prevKw > 1) {
            for (Kw paddingKw = prevKw + 1; paddingKw < kw; paddingKw++) {
                Range<Kw> paddingKwRange {paddingKw, paddingKw};
                SrcIDb1Doc dummyDoc {DUMMY, DUMMY_RANGE<IdAlias>(), paddingKwRange};
                DbEntry<SrcIDb1Doc, Kw> dummyDbEntry = DbEntry {dummyDoc, paddingKwRange};
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
            Range<Kw> paddingKwRange {maxDb1Kw, maxDb1Kw};
            SrcIDb1Doc dummyDoc {DUMMY, DUMMY_RANGE<IdAlias>(), paddingKwRange};
            DbEntry<SrcIDb1Doc, Kw> dummyDbEntry = DbEntry {dummyDoc, paddingKwRange};
            db1.push_back(dummyDbEntry);
        }
    }
    this->tdag1 = new TdagNode<Kw>(Range {db1KwBounds.first, maxDb1Kw});

    // replicate every document (in this case `SrcIDb1Doc`s) to all keyword ranges/TDAG 1 nodes that cover it
    stop = db1.size();
    db1.reserve(calcTdagItemCount(stop));
    for (long i = 0; i < stop; i++) {
        DbEntry<SrcIDb1Doc, Kw> dbEntry = db1[i];
        SrcIDb1Doc doc = dbEntry.first;
        Range<Kw> kwRange = dbEntry.second;
        std::list<Range<Kw>> ancestors = this->tdag1->getLeafAncestors(kwRange);
        for (Range<Kw> ancestor : ancestors) {
            if (ancestor == kwRange) {
                continue;
            }
            SrcIDb1Doc newDoc(doc.get(), ancestor);
            db1.push_back(std::pair {newDoc, ancestor});
        }
    }

    this->underly1->setup(secParam, db1);
}


template class LogSrcILoc<underly::PiBasLoc>;


//==============================================================================
// `PiBasLoc`
//==============================================================================


namespace underly {


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
PiBasLoc<DbDoc, DbKw>::~PiBasLoc() {
    this->clear();
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void PiBasLoc<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->clear();

    this->size = db.size();
    
    //--------------------------------------------------------------------------
    // generate keys

    this->prfKey = genKey(secParam);
    this->encKey = genKey(secParam);

    //--------------------------------------------------------------------------
    // build index

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
        std::vector<DbDoc> dbDocsWithSameDbKw = iter->second;
        long dbKwCount = dbDocsWithSameDbKw.size();
        this->dbKwCounts[dbKwRange] = dbKwCount;
        for (long dbKwCounter = 0; dbKwCounter < dbKwCount; dbKwCounter++) {
            DbDoc dbDoc = dbDocsWithSameDbKw[dbKwCounter];
            ustring label = findHash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken + toUstr(dbKwCounter));
            ustring iv = genIv(IV_LEN);
            ustring encDbDoc = padAndEncrypt(ENC_CIPHER, this->encKey, dbDoc.toUstr(), iv, EncIndBase::DOC_LEN - 1);
            this->encInd->write(
                label, std::pair {encDbDoc, iv}, dbKwRange, dbKwCount, dbKwCounter, this->minDbKw, this->leafCount
            );
        }
    }
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
std::vector<DbDoc> PiBasLoc<DbDoc, DbKw>::searchBase(const Range<DbKw>& query) const {
    auto iter = this->dbKwCounts.find(query);
    if (iter == this->dbKwCounts.end()) {
        return std::vector<DbDoc> {};
    }
    long dbKwCount = iter->second;

    std::vector<DbDoc> results;
    ustring queryToken = this->genQueryToken(query);
    for (long dbKwCounter = 0; dbKwCounter < dbKwCount; dbKwCounter++) {
        std::pair<ustring, ustring> encIndVal;
        this->encInd->find(query, dbKwCount, dbKwCounter, this->minDbKw, this->leafCount, encIndVal);
        ustring encDbDoc = encIndVal.first;
        ustring iv = encIndVal.second;
        // technically we decrypt in the client, but since there's no client-server distinction in this implementation
        // we'll just decrypt immediately to make the code cleaner
        ustring decDbDoc = decryptAndUnpad(ENC_CIPHER, this->encKey, encDbDoc, iv);
        DbDoc result = DbDoc::fromUstr(decDbDoc);
        results.push_back(result);
    }

    return results;
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void PiBasLoc<DbDoc, DbKw>::clear() {
    PiBasBase<DbDoc, DbKw>::clear();
    if (this->encInd != nullptr) {
        this->encInd->clear();
    }
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
bool PiBasLoc<DbDoc, DbKw>::readEncIndValFromPos(ulong pos, std::pair<ustring, ustring>& ret) const {
    return this->encInd->readValFromPos(pos, ret);
}


template class PiBasLoc<Doc<>, Kw>;       
template class PiBasLoc<SrcIDb1Doc, Kw>;
//template class PiBasLoc<Doc<IdAlias>, IdAlias>;


} // namespace `underly`
