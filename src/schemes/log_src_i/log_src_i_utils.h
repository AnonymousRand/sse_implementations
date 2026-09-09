#pragma once

#include <functional>

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/tuple.h"


namespace log_src_i::utils {


Db<Tuple<>> sortInputDbByKw(const Db<Tuple<>>& db);


/**
 * fill out leaf nodes for both DB 1 and DB 2.
 *
 * (note that we allow DB 1 to not be an actual `Db`, which is why this takes a lambda and not
 * a `Db` for DB 1.)
 */
void initDbsLeaves(
    const Db<Tuple<>>& sortedDb,
    Db<Tuple<IdAlias>>& db2,
    const std::function<
        void(Kw prevKw, IdAlias firstIdAliasWithKw, IdAlias lastIdAliasWithKw)
    >& addDb1Leaf
);


} // namespace `log_src_i::utils`
