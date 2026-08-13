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
    Db<Tuple<>> dbSorted = db;
    auto sortByKw = [](const Tuple<>& tuple1, const Tuple<>& tuple2) {
        return tuple1.getKw() < tuple2.getKw();
    };
    std::sort(dbSorted.begin(), dbSorted.end(), sortByKw);

    // assign index 2 nodes/"identifier aliases" and populate both `db1` and `db2`
    // leaves with this information
    Db<SrcIDb1Tuple> db1;
    Db<Tuple<IdAlias>> db2;
    db1.reserve(dbSorted.size());
    db2.reserve(dbSorted.size());
    Kw prevKw = DUMMY;
    IdAlias firstIdAliasWithKw;
    IdAlias lastIdAliasWithKw;
    auto addDb1Leaf = [&db1](Kw prevKw, IdAlias firstIdAliasWithKw, IdAlias lastIdAliasWithKw) {
        Range<IdAlias> idAliasRangeWithKw {firstIdAliasWithKw, lastIdAliasWithKw};
        Range<Kw> kwRange {prevKw, prevKw};
        SrcIDb1Tuple newTuple {prevKw, idAliasRangeWithKw, kwRange};
        db1.push_back(newTuple);
    };

    for (int64_t idAlias = 0; idAlias < dbSorted.size(); idAlias++) {
        Tuple<> tuple = dbSorted[idAlias];
        // populate `db2` leaves
        Range<IdAlias> idAliasRange {idAlias, idAlias};
        Tuple<IdAlias> newTuple(tuple.getDbDoc(), idAliasRange);
        db2.push_back(newTuple);

        // populate `db1` leaves
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
    // make sure to write in last `Kw` (which cannot be detected by `kw != prevKw` in
    // the loop above; note this relies on nothing in `db` having keyword `DUMMY`)
    if (prevKw != DUMMY) {
        addDb1Leaf(prevKw, firstIdAliasWithKw, lastIdAliasWithKw);
    }

    // build TDAG 2 over id aliases, padding the leaf count to the next power of 2
    // as is required for Log-SRC-i*
    this->buildTdag2(db2, true);

    // replicate every document to all id alias ranges/TDAG 2 nodes that cover it
    this->replicateDb<>(db2, this->tdag2);

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
    if (dbSorted.size() > 0) {
        Tuple<> tuple = dbSorted[0];
        prevKw = tuple.getKw();
        for (int64_t i = 1; i < dbSorted.size(); i++) {
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
    this->tdag1 = new TdagNode<Kw>(db1KwBounds.first, maxDb1Kw);

    // replicate every document to all keyword ranges/TDAG 1 nodes that cover it
    this->replicateDb<>(db1, this->tdag1);

    this->underly1->setup(secParam, db1);
}
