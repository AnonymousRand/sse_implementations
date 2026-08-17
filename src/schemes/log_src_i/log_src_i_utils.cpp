#include "schemes/log_src_i/log_src_i_utils.h"

#include <functional>

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


namespace log_src_i::utils {


Db<Tuple<>> sortInputDbByKw(const Db<Tuple<>>& db) {
    Db<Tuple<>> sortedDb = db;

    auto sortByKw = [](const Tuple<>& tuple1, const Tuple<>& tuple2) {
        return tuple1.getKw() < tuple2.getKw();
    };
    sortedDb.sort(sortByKw);
    return sortedDb;
}


void initDbsLeaves(
    const Db<Tuple<>>& sortedDb,
    Db<Tuple<IdAlias>>& db2,
    const std::function<
        void(Kw prevKw, IdAlias firstIdAliasWithKw, IdAlias lastIdAliasWithKw)
    >& addDb1Leaf
) {
    Kw prevKw = DUMMY;
    IdAlias firstIdAliasWithKw;
    IdAlias lastIdAliasWithKw;

    for (bigint idAlias = 0; idAlias < sortedDb.size(); idAlias++) {
        Tuple<> tuple = sortedDb[idAlias];
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
}


} // namespace `log_src_i::utils`
