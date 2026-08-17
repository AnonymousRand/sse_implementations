#include "schemes/log_src/log_src_utils.h"

#include <concepts>
#include <list>

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tdag.h"
#include "utils/types/tuple.h"


namespace log_src::utils {


template <IsDbTuple DbTuple>
void buildTdagDbFromLeaves(
    Db<DbTuple>& db, TdagNode<typename DbTuple::DbKwType>*& tdag, bool shouldPadDb
) {
    using DbKw = typename DbTuple::DbKwType;

    // obtain TDAG leaf bounds
    Range<DbKw> dbKwBounds = db.findDbKwBounds();
    DbKw maxDbKw = dbKwBounds.second;

    // pad if necessary
    if (shouldPadDb) {
        db.pad(maxDbKw);
    }

    // construct TDAG
    tdag = new TdagNode<DbKw>(dbKwBounds.first, maxDbKw);
    
    // replicate every (leaf) DB tuple to all TDAG nodes that cover it
    replTdagDb<DbTuple>(db, tdag);
}


template <IsDbTuple DbTuple>
void replTdagDb(Db<DbTuple>& db, const TdagNode<typename DbTuple::DbKwType>* tdag) {
    using DbKw = typename DbTuple::DbKwType;

    bigint dbSize = db.size();
    db.reserve(dbSize + ::utils::tdag::calcTdagTupleCount(dbSize));
    for (bigint i = 0; i < dbSize; i++) {
        DbTuple tuple = db[i];
        Range<DbKw> dbKwRange = tuple.getDbKwRange();
        std::list<Range<DbKw>> ancestors = tdag->getLeafAncestors(dbKwRange);
        for (const Range<DbKw>& ancestor : ancestors) {
            if (ancestor == dbKwRange) {
                continue;
            }
            DbTuple newTuple(tuple.getDbDoc(), ancestor);
            db.push_back(newTuple);
        }
    }
}


//------------------------------------------------------------------------------
// explicit template instantiations


template void buildTdagDbFromLeaves<Tuple<>>(
    Db<Tuple<>>& db, TdagNode<Kw>*& tdag, bool shouldPadDb
);
template void buildTdagDbFromLeaves<SrcIDb1Tuple>(
    Db<SrcIDb1Tuple>& db, TdagNode<Kw>*& tdag, bool shouldPadDb
);
//template void buildTdagDbFromLeaves<Tuple<IdAlias>>(
//    Db<Tuple<IdAlias>>& db, TdagNode<IdAlias>*& tdag, bool shouldPadDb
//);


template void replTdagDb<Tuple<>>(Db<Tuple<>>& db, const TdagNode<Kw>* tdag);
template void replTdagDb<SrcIDb1Tuple>(Db<SrcIDb1Tuple>& db, const TdagNode<Kw>* tdag);
//template void replTdagDb<Tuple<IdAlias>>(Db<Tuple<IdAlias>>& db, const TdagNode<IdAlias>* tdag);


} // namespace `log_src::utils`
