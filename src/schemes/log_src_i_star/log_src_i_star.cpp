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


void LogSrcIStar::setup(int secParam, const Db<Tuple<>, Kw>& db) {
    this->clear();

    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();

    //--------------------------------------------------------------------------
    // build index 2

    // sort documents by keyword
    auto sortByKw = [](const DbTuple<Tuple<>, Kw>& dbTuple1, const DbTuple<Tuple<>, Kw>& dbTuple2) {
        return dbTuple1.first.getKw() < dbTuple2.first.getKw();
    };
    Db<Tuple<>, Kw> dbSorted = db;
    std::sort(dbSorted.begin(), dbSorted.end(), sortByKw);

    // assign index 2 nodes/"identifier aliases" and populate both `db1` and `db2`
    // leaves with this information
    Db<SrcIDb1Tuple, Kw> db1;
    Db<Tuple<IdAlias>, IdAlias> db2;
    int64_t dbSortedSize = dbSorted.size();
    db1.reserve(dbSortedSize);
    db2.reserve(dbSortedSize);
    Kw prevKw = DUMMY;
    IdAlias firstIdAliasWithKw;
    IdAlias lastIdAliasWithKw;
    auto addDb1Leaf = [&db1](Kw prevKw, IdAlias firstIdAliasWithKw, IdAlias lastIdAliasWithKw) {
        Range<IdAlias> idAliasRangeWithKw {firstIdAliasWithKw, lastIdAliasWithKw};
        Range<Kw> kwRange {prevKw, prevKw};
        SrcIDb1Tuple newTuple {prevKw, idAliasRangeWithKw, kwRange};
        DbTuple<SrcIDb1Tuple, Kw> newDbTuple {newTuple, kwRange};
        db1.push_back(newDbTuple);
    };
    for (int64_t idAlias = 0; idAlias < dbSortedSize; idAlias++) {
        DbTuple<Tuple<>, Kw> dbTuple = dbSorted[idAlias];
        Tuple<> tuple = dbTuple.first;
        Kw kw = dbTuple.second.first; // tuples in `db` must have size 1 `Kw` ranges!
        // populate `db2` leaves
        Range<IdAlias> idAliasRange {idAlias, idAlias};
        Tuple<IdAlias> newDb2Tuple(tuple.get(), idAliasRange);
        DbTuple<Tuple<IdAlias>, IdAlias> newDb2Tuple = DbTuple {newDb2Tuple, idAliasRange};
        db2.push_back(newDb2Tuple);

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
    for (DbTuple<Tuple<IdAlias>, IdAlias> dbTuple : db2) {
        IdAlias idAlias = dbTuple.second.first;
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
            Tuple<IdAlias> dummyTuple = Tuple<IdAlias>::genDummy(idAliasRange);
            DbTuple<Tuple<IdAlias>, IdAlias> dummyDbTuple = DbTuple {dummyTuple, idAliasRange};
            db2.push_back(dummyDbTuple);
        }
    }
    this->tdag2 = new TdagNode<IdAlias>(Range<IdAlias> {0, maxIdAlias});

    // replicate every document to all id alias ranges/TDAG 2 nodes that cover it
    db2Size = db2.size();
    db2.reserve(utils::calcTdagTupleCount(db2Size));
    for (int64_t i = 0; i < db2Size; i++) {
        DbTuple<Tuple<IdAlias>, IdAlias> dbTuple = db2[i];
        Tuple<IdAlias> tuple = dbTuple.first;
        Range<IdAlias> idAliasRange = dbTuple.second;
        std::list<Range<IdAlias>> ancestors = this->tdag2->getLeafAncestors(idAliasRange);
        for (Range<IdAlias> ancestor : ancestors) {
            if (ancestor == idAliasRange) {
                continue;
            }
            Tuple<IdAlias> newTuple(tuple.get(), ancestor);
            db2.push_back(std::pair {newTuple, ancestor});
        }
    }

    this->underly2->setup(secParam, db2);

    //--------------------------------------------------------------------------
    // build index 1

    // build TDAG 1 over `Kw`s
    // since `Kw`s have no guarantee of being contiguous but the leaves and hence
    // bottom level in the index must be, we need to pad `db1` to have (exactly)
    // one tuple per `Kw` (we can just leave blanks in the case of non-locality
    // Log-SRC-i since tuples are placed pseudorandomly in the index, but here we
    // have to pad to avoid empty buckets in the index that the server knows
    // corresponds to a lack of tuples with that keyword)
    DbTuple<Tuple<>, Kw> dbTuple = dbSorted[0];
    prevKw = dbTuple.second.first;
    for (int64_t i = 1; i < dbSortedSize; i++) {
        dbTuple = dbSorted[i];
        Kw kw = dbTuple.second.first;
        // if non-contiguous `Kw`s detected, fill in the gap with dummies
        if (kw - prevKw > 1) {
            for (Kw paddingKw = prevKw + 1; paddingKw < kw; paddingKw++) {
                Range<Kw> paddingKwRange {paddingKw, paddingKw};
                SrcIDb1Tuple dummyTuple = SrcIDb1Tuple::genDummy(paddingKwRange);
                DbTuple<SrcIDb1Tuple, Kw> dummyDbTuple = DbTuple {dummyTuple, paddingKwRange};
                db1.push_back(dummyDbTuple);
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
            SrcIDb1Tuple dummyTuple = SrcIDb1Tuple::genDummy(paddingKwRange);
            DbTuple<SrcIDb1Tuple, Kw> dummyDbTuple = DbTuple {dummyTuple, paddingKwRange};
            db1.push_back(dummyDbTuple);
        }
    }
    this->tdag1 = new TdagNode<Kw>(Range {db1KwBounds.first, maxDb1Kw});

    // replicate every document (in this case `SrcIDb1Tuple`s) to all keyword ranges/
    // TDAG 1 nodes that cover it
    db1Size = db1.size();
    db1.reserve(utils::calcTdagTupleCount(db1Size));
    for (int64_t i = 0; i < db1Size; i++) {
        DbTuple<SrcIDb1Tuple, Kw> dbTuple = db1[i];
        SrcIDb1Tuple tuple = dbTuple.first;
        Range<Kw> kwRange = dbTuple.second;
        std::list<Range<Kw>> ancestors = this->tdag1->getLeafAncestors(kwRange);
        for (Range<Kw> ancestor : ancestors) {
            if (ancestor == kwRange) {
                continue;
            }
            SrcIDb1Tuple newTuple(tuple.get(), ancestor);
            db1.push_back(std::pair {newTuple, ancestor});
        }
    }

    this->underly1->setup(secParam, db1);
}
