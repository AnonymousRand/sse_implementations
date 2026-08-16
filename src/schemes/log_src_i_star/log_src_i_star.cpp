#include "schemes/log_src_i_star/log_src_i_star.h"

#include "schemes/log_src_i_star/underly.h" 

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tdag.h"
#include "utils/types/tuple.h"


//------------------------------------------------------------------------------
// `ISse`


void LogSrcIStar::setup(int secParam, const Db<Tuple<>>& db) {
    this->clear();

    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();

    //--------------------------------------------------------------------------
    // init sub-DBs

    // sort documents by keyword
    Db<Tuple<>> sortedDb = LogSrcIBase<log_src_i_star::Underly>::sortInputDb(db);

    // assign index 2 nodes/"identifier aliases" and populate both `db1` and `db2`
    // leaves with this information
    Db<SrcIDb1Tuple> db1;
    Db<Tuple<IdAlias>> db2;
    db1.reserve(sortedDb.size());
    db2.reserve(sortedDb.size());
    auto addDb1Leaf = [&db1](Kw prevKw, IdAlias firstIdAliasWithKw, IdAlias lastIdAliasWithKw) {
        Range<IdAlias> idAliasRangeWithKw {firstIdAliasWithKw, lastIdAliasWithKw};
        Range<Kw> kwRange {prevKw, prevKw};
        SrcIDb1Tuple newTuple {prevKw, idAliasRangeWithKw, kwRange};
        db1.push_back(newTuple);
    };
    LogSrcIBase<log_src_i_star::Underly>::initDbsLeaves(sortedDb, db2, addDb1Leaf);

    //--------------------------------------------------------------------------
    // build index 1

    // first ensure (leaf) `Kw`s are contiguous:
    // since `Kw`s have no guarantee of being contiguous but the leaves and hence
    // bottom level in the index must be, we need to pad `db1` to have (exactly)
    // one tuple per `Kw` (we can just leave blanks in the case of non-locality
    // Log-SRC-i since tuples are placed pseudorandomly in the index, but here we
    // have to pad to avoid empty buckets in the index that the server knows
    // corresponds to a lack of tuples with that keyword)
    if (sortedDb.size() > 0) {
        Tuple<> tuple = sortedDb[0];
        Kw prevKw = tuple.getKw();
        for (bigint i = 1; i < sortedDb.size(); i++) {
            tuple = sortedDb[i];
            Kw kw = tuple.getKw();
            // if non-contiguous `Kw`s detected, fill in the gap with dummies
            if (kw - prevKw > 1) {
                for (Kw paddingKw = prevKw + 1; paddingKw < kw; paddingKw++) {
                    Range<Kw> paddingKwRange {paddingKw, paddingKw};
                    SrcIDb1Tuple dummyTuple = SrcIDb1Tuple::DUMMY(paddingKwRange);
                    db1.push_back(dummyTuple);
                }
            }
            prevKw = kw;
        }
    }

    // after guaranteeing contiguous-ness of `Kw`s, build TDAG 1 over `Kw`s and replicate
    // `db1` appropriately, again padding the leaf count to the next power of 2 as is
    // required for Log-SRC-i*
    utils::buildTdagAndDb<SrcIDb1Tuple>(this->tdag1, db1, true);

    this->underly1->setup(secParam, db1);

    //--------------------------------------------------------------------------
    // build index 2

    // build TDAG 2 over `IdAlias`es and replicate `db2` appropriately, and padding
    // the leaf count to the next power of 2 as is required for Log-SRC-i*
    utils::buildTdagAndDb<Tuple<IdAlias>>(this->tdag2, db2, true);

    this->underly2->setup(secParam, db2);
}
