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
#include "utils/types.h"


//------------------------------------------------------------------------------
// `ISse`


void LogSrcIStar::setup(int secParam, const Db<Tuple<>>& db) {
    this->clear();

    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();

    //--------------------------------------------------------------------------
    // build index 2

    // sort documents by keyword
    auto sortByKw = [](const Tuple<>& tuple1, const Tuple<>& tuple2) {
        return tuple1.getKw() < tuple2.getKw();
    };
    Db<Tuple<>> dbSorted = db;
    std::sort(dbSorted.begin(), dbSorted.end(), sortByKw);

    // assign index 2 nodes/"identifier aliases" and populate both `db1` and `db2`
    // leaves with this information
    Db<SrcIDb1Tuple> db1;
    Db<Tuple<IdAlias>> db2;
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
        db1.push_back(newTuple);
    };

    for (int64_t idAlias = 0; idAlias < dbSortedSize; idAlias++) {
        Tuple<> tuple = dbSorted[idAlias];
        // populate `db2` leaves
        Range<IdAlias> idAliasRange {idAlias, idAlias};
        Tuple<IdAlias> newTuple(tuple.getDbDoc(), idAliasRange);
        db2.push_back(newTuple);

        // populate `db1` leaves
        // TODO this may? need to be getDbKw().first
        Kw kw = tuple.getKw();
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
    for (Tuple<IdAlias> tuple: db2) {
        IdAlias idAlias = tuple.getDbKwRange().first; // must be size 1 range
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
            db2.push_back(dummyTuple);
        }
    }
    // TODO make another constructor for tdagnode that just takes the start and end instead of range?
    this->tdag2 = new TdagNode<IdAlias>(Range<IdAlias> {0, maxIdAlias});

    // replicate every document to all id alias ranges/TDAG 2 nodes that cover it
    db2Size = db2.size();
    db2.reserve(utils::calcTdagTupleCount(db2Size));
    for (int64_t i = 0; i < db2Size; i++) {
        Tuple<IdAlias> tuple = db2[i];
        // TODO: is SrcIDb1Tuple::getIdAliasRange() used?
        Range<IdAlias> idAliasRange = tuple.getDbKwRange();
        std::list<Range<IdAlias>> ancestors = this->tdag2->getLeafAncestors(idAliasRange);
        for (Range<IdAlias> ancestor : ancestors) {
            if (ancestor == idAliasRange) {
                continue;
            }
            Tuple<IdAlias> newTuple(tuple.getDbDoc(), ancestor);
            db2.push_back(newTuple);
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
    Tuple<> tuple = dbSorted[0];
    // TODO this may? need to be getDbKwRange().first
    prevKw = tuple.getKw();
    for (int64_t i = 1; i < dbSortedSize; i++) {
        tuple = dbSorted[i];
        Kw kw = tuple.getKw();
        // if non-contiguous `Kw`s detected, fill in the gap with dummies
        if (kw - prevKw > 1) {
            for (Kw paddingKw = prevKw + 1; paddingKw < kw; paddingKw++) {
                Range<Kw> paddingKwRange {paddingKw, paddingKw};
                SrcIDb1Tuple dummyTuple = SrcIDb1Tuple::genDummy(paddingKwRange);
                db1.push_back(dummyTuple);
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
            db1.push_back(dummyTuple);
        }
    }
    this->tdag1 = new TdagNode<Kw>(Range {db1KwBounds.first, maxDb1Kw});

    // replicate every document (in this case `SrcIDb1Tuple`s) to all keyword ranges/
    // TDAG 1 nodes that cover it
    db1Size = db1.size();
    db1.reserve(utils::calcTdagTupleCount(db1Size));
    for (int64_t i = 0; i < db1Size; i++) {
        SrcIDb1Tuple tuple = db1[i];
        Range<Kw> kwRange = tuple.getDbKwRange();
        std::list<Range<Kw>> ancestors = this->tdag1->getLeafAncestors(kwRange);
        for (Range<Kw> ancestor : ancestors) {
            if (ancestor == kwRange) {
                continue;
            }
            SrcIDb1Tuple newTuple(tuple.getDbDoc(), ancestor);
            db1.push_back(newTuple);
        }
    }

    this->underly1->setup(secParam, db1);
}
