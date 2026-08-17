#include "schemes/log_src_i/log_src_i.h"

#include <concepts>

#include "schemes/interfaces/sse.h"
#include "schemes/log_src_i/log_src_i_base.h"

// for explicit template instantiation
#include "schemes/n_log_n/n_log_n.h"
#include "schemes/pi_bas/pi_bas.h"

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tdag.h"
#include "utils/types/tuple.h"


//------------------------------------------------------------------------------
// `ISse`


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
void LogSrcI<Underly>::setup(int secParam, const Db<Tuple<>>& db) {
    this->clear();

    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();

    //--------------------------------------------------------------------------
    // init sub-DBs

    // sort documents by keyword
    Db<Tuple<>> sortedDb = LogSrcIBase<Underly>::sortInputDb(db);

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
    LogSrcIBase<Underly>::initDbsLeaves(sortedDb, db2, addDb1Leaf);

    //--------------------------------------------------------------------------
    // build index 1

    // build TDAG 1 over `Kw`s and replicate `db1` appropriately
    utils::tdag::buildTdagAndDb<SrcIDb1Tuple>(this->tdag1, db1);

    this->underly1->setup(secParam, db1);

    //--------------------------------------------------------------------------
    // build index 2

    // build TDAG 2 over `IdAlias`es and replicate `db2` appropriately
    utils::tdag::buildTdagAndDb<Tuple<IdAlias>>(this->tdag2, db2);

    this->underly2->setup(secParam, db2);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class LogSrcI<PiBas>;
template class LogSrcI<NLogN>;
