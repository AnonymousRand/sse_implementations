#include "schemes/log_src_i_star/log_src_i_star.h"

#include "schemes/log_src/log_src_utils.h"
#include "schemes/log_src_i/log_src_i_base.h"
#include "schemes/log_src_i/log_src_i_utils.h"
#include "schemes/log_src_i_star/log_src_i_star_underly.h"

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
    this->size = db.getSize();

    //--------------------------------------------------------------------------
    // init sub-DBs

    // sort documents by keyword
    Db<Tuple<>> sortedDb = log_src_i::utils::sortInputDbByKw(db);

    // assign index 2 nodes/"identifier aliases" and populate both `db1` and `db2`
    // leaves with this information
    Db<SrcIDb1Tuple> db1;
    Db<Tuple<IdAlias>> db2;
    db1.reserve(sortedDb.getSize());
    db2.reserve(sortedDb.getSize());
    auto addDb1Leaf = [&db1](Kw prevKw, IdAlias firstIdAliasWithKw, IdAlias lastIdAliasWithKw) {
        Range<IdAlias> idAliasRangeWithKw {firstIdAliasWithKw, lastIdAliasWithKw};
        Range<Kw> kwRange {prevKw, prevKw};
        SrcIDb1Tuple newTuple {prevKw, idAliasRangeWithKw, kwRange};
        db1.append(newTuple);
    };
    log_src_i::utils::initDbsLeaves(sortedDb, db2, addDb1Leaf);

    //--------------------------------------------------------------------------
    // build index 1

    // first ensure (leaf) `Kw`s are contiguous:
    // since `Kw`s have no guarantee of being contiguous but the leaves and hence
    // bottom level in the index must be, we need to pad `db1` to have (exactly)
    // one tuple per `Kw` (we can just leave blanks in the case of non-locality
    // Log-SRC-i since tuples are placed pseudorandomly in the index, but here we
    // have to pad to avoid empty buckets in the index that the server knows
    // corresponds to a lack of tuples with that keyword)
    if (sortedDb.getSize() > 0) {
        Tuple<> tuple = sortedDb[0];
        Kw prevKw = tuple.getKw();
        for (bigint i = 1; i < sortedDb.getSize(); i++) {
            tuple = sortedDb[i];
            Kw kw = tuple.getKw();
            // if non-contiguous `Kw`s detected, fill in the gap with dummies
            if (kw - prevKw > 1) {
                for (Kw paddingKw = prevKw + 1; paddingKw < kw; paddingKw++) {
                    Range<Kw> paddingKwRange {paddingKw, paddingKw};
                    SrcIDb1Tuple dummyTuple = SrcIDb1Tuple::DUMMY(paddingKwRange);
                    db1.append(dummyTuple);
                }
            }
            prevKw = kw;
        }
    }

    // after guaranteeing contiguous-ness of `Kw`s, build TDAG 1 over `Kw`s and replicate
    // `db1` appropriately, again padding the leaf count to the next power of 2 as is
    // required for Log-SRC-i*
    log_src::utils::buildTdagDbFromLeaves<SrcIDb1Tuple>(db1, this->tdag1, true);

    this->underly1->setup(secParam, db1);

    //--------------------------------------------------------------------------
    // build index 2

    // build TDAG 2 over `IdAlias`es and replicate `db2` appropriately, and padding
    // the leaf count to the next power of 2 as is required for Log-SRC-i*
    log_src::utils::buildTdagDbFromLeaves<Tuple<IdAlias>>(db2, this->tdag2, true);

    this->underly2->setup(secParam, db2);
}
