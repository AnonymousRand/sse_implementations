#include "log_src_i_star.h"

#include "utils/cryptography.h"


//==============================================================================
// `LogSrcIStarUnderly`
//==============================================================================


namespace underly {


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void LogSrcIStarUnderly<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    Range<DbKw> dbKwBounds = findDbKwBounds(db);
    this->leafCount = dbKwBounds.size();
    Nlogn<DbDoc, DbKw>::setup(secParam, db);
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
std::vector<DbDoc> LogSrcIStarUnderly<DbDoc, DbKw>::searchBase(const Range<DbKw>& query) const {
    std::vector<DbDoc> results;

    // PRF(K_1, w)
    ustring queryToken = this->genQueryToken(query);

    // for Log-SRC-i*, the TDAG structure means we can determine the number of results and hence the level to search
    // based on the size of the queried range/node, so we don't have to store an encrypted map
    // (and result size is leaked to server anyway)
    long dbKwListSize = query.size();
    long dbKwListPaddedSize = std::pow(2, std::ceil(std::log2(dbKwListSize))); // this is bucket size

    // compute `lvl` and `pos` of correct bucket (the same way as in `setup()`)
    ustring label;
    std::pair<ulong, ulong> lvlAndPos = this->map(queryToken, dbKwListSize, label);
    ulong lvl = lvlAndPos.first;
    ulong pos = lvlAndPos.second;
    // return entire bucket (`dbKwListPaddedSize` instead of `dbKwListSize`) from server to hide true result size
    ulong startPos = pos * this->computeBcktSizeOnLvl(lvl);
    for (long dbKwCounter = 0; dbKwCounter < dbKwListPaddedSize; dbKwCounter++) {
        EncIndVal encIndVal;
        bool isFound;
        if (dbKwCounter == 0) {
            // if first read, get the right bucket start pos (e.g. in case of hash/modulo collision in encrypted index)
            // note: dummies must also use the correct (not dummy) `label` so they are still found by `find()`
            isFound = this->encIndLvls[lvl]->find(startPos, label, encIndVal, &startPos);
        } else {
            // after first read, just read from the bucket consecutively as we are now guaranteed consecutivity
            isFound = this->encIndLvls[lvl]->read(startPos + dbKwCounter, encIndVal);
        }
        if (!isFound) {
            break;
        }

        DbDoc result = this->decryptEncIndVal(encIndVal);
        results.push_back(result);
    }

    return results;
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
long LogSrcIStarUnderly<DbDoc, DbKw>::computeNumLvls() const {
    // the key to avoiding the blowup of using NlogN as a black box is by using `leafCount` instead of `db.size()` here,
    // since `db.size()` includes the replicated tuples and using it sort of assumes those are only the "raw" entries
    return std::ceil(std::log2(this->leafCount)) + 1;
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
long LogSrcIStarUnderly<DbDoc, DbKw>::computeBcktCountOnLvl(long lvlNum) const {
    if (lvlNum == 0) {
        return this->leafCount;
    } else {
        // this gives the TDAG node/bucket count at level `lvlNum` (for `lvlNum` >= 1)
        return std::pow(2, this->numLvls - lvlNum) - 1;
    }
}


template class LogSrcIStarUnderly<Doc<>, Kw>;       
template class LogSrcIStarUnderly<SrcIDb1Doc, Kw>;
//template class LogSrcIStarUnderly<Doc<IdAlias>, IdAlias>;


} // namespace `underly`


//==============================================================================
// `LogSrcIStar`
//==============================================================================


void LogSrcIStar::setup(int secParam, const Db<Doc<>, Kw>& db) {
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
    long dbSortedSize = dbSorted.size();
    db1.reserve(dbSortedSize);
    db2.reserve(dbSortedSize);
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
    for (long idAlias = 0; idAlias < dbSortedSize; idAlias++) {
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
    // pad TDAG 2 leaf count to the next power of two, as is required for Log-SRC-i*
    long db2Size = db2.size();
    if (!std::has_single_bit((ulong)db2Size)) {
        long amountToPad = std::pow(2, std::ceil(std::log2(db2Size))) - db2Size;
        db2.reserve(db2Size + amountToPad);
        for (long i = 0; i < amountToPad; i++) {
            maxIdAlias++;
            Range<IdAlias> idAliasRange {maxIdAlias, maxIdAlias};
            Doc<IdAlias> dummyDoc = Doc<IdAlias>::genDummy(idAliasRange);
            DbEntry<Doc<IdAlias>, IdAlias> dummyDbEntry = DbEntry {dummyDoc, idAliasRange};
            db2.push_back(dummyDbEntry);
        }
    }
    this->tdag2 = new TdagNode<IdAlias>(Range<IdAlias> {0, maxIdAlias});

    // replicate every document to all id alias ranges/TDAG 2 nodes that cover it
    db2Size = db2.size();
    db2.reserve(calcTdagItemCount(db2Size));
    for (long i = 0; i < db2Size; i++) {
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
    for (long i = 1; i < dbSortedSize; i++) {
        dbEntry = dbSorted[i];
        Kw kw = dbEntry.second.first;
        // if non-contiguous `Kw`s detected, fill in the gap with dummies
        if (kw - prevKw > 1) {
            for (Kw paddingKw = prevKw + 1; paddingKw < kw; paddingKw++) {
                Range<Kw> paddingKwRange {paddingKw, paddingKw};
                SrcIDb1Doc dummyDoc = SrcIDb1Doc::genDummy(paddingKwRange);
                DbEntry<SrcIDb1Doc, Kw> dummyDbEntry = DbEntry {dummyDoc, paddingKwRange};
                db1.push_back(dummyDbEntry);
            }
        }
        prevKw = kw;
    }
    // after guaranteeing contiguous-ness of `Kw`s, pad `db1` to power of 2 as well
    long db1Size = db1.size();
    Range<Kw> db1KwBounds = findDbKwBounds(db1);
    Kw maxDb1Kw = db1KwBounds.second;
    if (!std::has_single_bit((ulong)db1Size)) {
        long amountToPad = std::pow(2, std::ceil(std::log2(db1Size))) - db1Size;
        db1.reserve(db1Size + amountToPad);
        for (long i = 0; i < amountToPad; i++) {
            maxDb1Kw++;
            Range<Kw> paddingKwRange {maxDb1Kw, maxDb1Kw};
            SrcIDb1Doc dummyDoc = SrcIDb1Doc::genDummy(paddingKwRange);
            DbEntry<SrcIDb1Doc, Kw> dummyDbEntry = DbEntry {dummyDoc, paddingKwRange};
            db1.push_back(dummyDbEntry);
        }
    }
    this->tdag1 = new TdagNode<Kw>(Range {db1KwBounds.first, maxDb1Kw});

    // replicate every document (in this case `SrcIDb1Doc`s) to all keyword ranges/TDAG 1 nodes that cover it
    db1Size = db1.size();
    db1.reserve(calcTdagItemCount(db1Size));
    for (long i = 0; i < db1Size; i++) {
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
