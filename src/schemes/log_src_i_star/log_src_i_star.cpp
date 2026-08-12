#include "schemes/log_src_i_star/log_src_i_star.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <list>
#include <utility>

#include "schemes/log_src_i_star/underly.h" 

#include "utils/db.h"
#include "utils/range.h"
#include "utils/sse_utils.h"
#include "utils/tdag.h"


//------------------------------------------------------------------------------
// `ISse`


void LogSrcIStar::setup(int secParam, const Db<Record<>, Kw>& db) {
    this->clear();

    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();

    //--------------------------------------------------------------------------
    // build index 2

    // sort documents by keyword
    auto sortByKw = [](const DbRecord<Record<>, Kw>& dbRecord1, const DbRecord<Record<>, Kw>& dbRecord2) {
        return dbRecord1.first.getKw() < dbRecord2.first.getKw();
    };
    Db<Record<>, Kw> dbSorted = db;
    std::sort(dbSorted.begin(), dbSorted.end(), sortByKw);

    // assign index 2 nodes/"identifier aliases" and populate both `db1` and `db2`
    // leaves with this information
    Db<SrcIDb1Record, Kw> db1;
    Db<Record<IdAlias>, IdAlias> db2;
    int64_t dbSortedSize = dbSorted.size();
    db1.reserve(dbSortedSize);
    db2.reserve(dbSortedSize);
    Kw prevKw = DUMMY;
    IdAlias firstIdAliasWithKw;
    IdAlias lastIdAliasWithKw;
    auto addDb1Leaf = [&db1](Kw prevKw, IdAlias firstIdAliasWithKw, IdAlias lastIdAliasWithKw) {
        Range<IdAlias> idAliasRangeWithKw {firstIdAliasWithKw, lastIdAliasWithKw};
        Range<Kw> kwRange {prevKw, prevKw};
        SrcIDb1Record newRecord {prevKw, idAliasRangeWithKw, kwRange};
        DbRecord<SrcIDb1Record, Kw> newDbRecord {newRecord, kwRange};
        db1.push_back(newDbRecord);
    };
    for (int64_t idAlias = 0; idAlias < dbSortedSize; idAlias++) {
        DbRecord<Record<>, Kw> dbRecord = dbSorted[idAlias];
        Record<> record = dbRecord.first;
        Kw kw = dbRecord.second.first; // records in `db` must have size 1 `Kw` ranges!
        // populate `db2` leaves
        Range<IdAlias> idAliasRange {idAlias, idAlias};
        Record<IdAlias> newDb2Record(record.get(), idAliasRange);
        DbRecord<Record<IdAlias>, IdAlias> newDb2Record = DbRecord {newDb2Record, idAliasRange};
        db2.push_back(newDb2Record);

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
    // make sure to write in last `Kw` (which cannot be detected by `kw != prevKw`
    // in the loop above)
    if (prevKw != DUMMY) {
        addDb1Leaf(prevKw, firstIdAliasWithKw, lastIdAliasWithKw);
    }

    // build TDAG 2 over id aliases
    IdAlias maxIdAlias = 0;
    for (DbRecord<Record<IdAlias>, IdAlias> dbRecord : db2) {
        IdAlias idAlias = dbRecord.second.first;
        if (idAlias > maxIdAlias) {
            maxIdAlias = idAlias;
        }
    }
    // pad TDAG 2 leaf count to the next power of two, as is required for Log-SRC-i*
    int64_t db2Size = db2.size();
    if (!std::has_single_bit((uint64_t)db2Size)) {
        int64_t amountToPad = std::pow(2, std::ceil(std::log2(db2Size))) - db2Size;
        db2.reserve(db2Size + amountToPad);
        for (int64_t i = 0; i < amountToPad; i++) {
            maxIdAlias++;
            Range<IdAlias> idAliasRange {maxIdAlias, maxIdAlias};
            Record<IdAlias> dummyRecord = Record<IdAlias>::genDummy(idAliasRange);
            DbRecord<Record<IdAlias>, IdAlias> dummyDbRecord = DbRecord {dummyRecord, idAliasRange};
            db2.push_back(dummyDbRecord);
        }
    }
    this->tdag2 = new TdagNode<IdAlias>(Range<IdAlias> {0, maxIdAlias});

    // replicate every document to all id alias ranges/TDAG 2 nodes that cover it
    db2Size = db2.size();
    db2.reserve(utils::calcTdagRecordCount(db2Size));
    for (int64_t i = 0; i < db2Size; i++) {
        DbRecord<Record<IdAlias>, IdAlias> dbRecord = db2[i];
        Record<IdAlias> record = dbRecord.first;
        Range<IdAlias> idAliasRange = dbRecord.second;
        std::list<Range<IdAlias>> ancestors = this->tdag2->getLeafAncestors(idAliasRange);
        for (Range<IdAlias> ancestor : ancestors) {
            if (ancestor == idAliasRange) {
                continue;
            }
            Record<IdAlias> newRecord(record.get(), ancestor);
            db2.push_back(std::pair {newRecord, ancestor});
        }
    }

    this->underly2->setup(secParam, db2);

    //--------------------------------------------------------------------------
    // build index 1

    // build TDAG 1 over `Kw`s
    // since `Kw`s have no guarantee of being contiguous but the leaves and hence
    // bottom level in the index must be, we need to pad `db1` to have (exactly)
    // one record per `Kw` (we can just leave blanks in the case of non-locality
    // Log-SRC-i since records are placed pseudorandomly in the index, but here we
    // have to pad to avoid empty buckets in the index that the server knows
    // corresponds to a lack of records with that keyword)
    DbRecord<Record<>, Kw> dbRecord = dbSorted[0];
    prevKw = dbRecord.second.first;
    for (int64_t i = 1; i < dbSortedSize; i++) {
        dbRecord = dbSorted[i];
        Kw kw = dbRecord.second.first;
        // if non-contiguous `Kw`s detected, fill in the gap with dummies
        if (kw - prevKw > 1) {
            for (Kw paddingKw = prevKw + 1; paddingKw < kw; paddingKw++) {
                Range<Kw> paddingKwRange {paddingKw, paddingKw};
                SrcIDb1Record dummyRecord = SrcIDb1Record::genDummy(paddingKwRange);
                DbRecord<SrcIDb1Record, Kw> dummyDbRecord = DbRecord {dummyRecord, paddingKwRange};
                db1.push_back(dummyDbRecord);
            }
        }
        prevKw = kw;
    }
    // after guaranteeing contiguous-ness of `Kw`s, pad `db1` to power of 2 as well
    int64_t db1Size = db1.size();
    Range<Kw> db1KwBounds = utils::findDbKwBounds(db1);
    Kw maxDb1Kw = db1KwBounds.second;
    if (!std::has_single_bit((uint64_t)db1Size)) {
        int64_t amountToPad = std::pow(2, std::ceil(std::log2(db1Size))) - db1Size;
        db1.reserve(db1Size + amountToPad);
        for (int64_t i = 0; i < amountToPad; i++) {
            maxDb1Kw++;
            Range<Kw> paddingKwRange {maxDb1Kw, maxDb1Kw};
            SrcIDb1Record dummyRecord = SrcIDb1Record::genDummy(paddingKwRange);
            DbRecord<SrcIDb1Record, Kw> dummyDbRecord = DbRecord {dummyRecord, paddingKwRange};
            db1.push_back(dummyDbRecord);
        }
    }
    this->tdag1 = new TdagNode<Kw>(Range {db1KwBounds.first, maxDb1Kw});

    // replicate every document (in this case `SrcIDb1Record`s) to all keyword ranges/
    // TDAG 1 nodes that cover it
    db1Size = db1.size();
    db1.reserve(utils::calcTdagRecordCount(db1Size));
    for (int64_t i = 0; i < db1Size; i++) {
        DbRecord<SrcIDb1Record, Kw> dbRecord = db1[i];
        SrcIDb1Record record = dbRecord.first;
        Range<Kw> kwRange = dbRecord.second;
        std::list<Range<Kw>> ancestors = this->tdag1->getLeafAncestors(kwRange);
        for (Range<Kw> ancestor : ancestors) {
            if (ancestor == kwRange) {
                continue;
            }
            SrcIDb1Record newRecord(record.get(), ancestor);
            db1.push_back(std::pair {newRecord, ancestor});
        }
    }

    this->underly1->setup(secParam, db1);
}
