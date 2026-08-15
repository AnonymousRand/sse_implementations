#include "sse_utils.h"

#include <bit>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <unordered_set>
#include <utility>
#include <vector>

#include "utils/db.h"
#include "utils/range.h"
#include "utils/tuple.h"
#include "utils/types.h"
#include "utils/ustring.h"


namespace utils {


template <IsDbTuple DbTuple>
Ind<DbTuple> genInd(const Db<DbTuple>& db, bool shouldShuffleKwLists) {
    using DbKw = typename DbTuple::DbKwType;

    Ind<DbTuple> ind;
    for (DbTuple dbTuple : db) {
        Range<DbKw> dbKwRange = dbTuple.getDbKwRange();
        if (ind.count(dbKwRange) == 0) {
            ind[dbKwRange] = Db<DbTuple> {dbTuple};
        } else {
            ind[dbKwRange].push_back(dbTuple);
        }
    }

    if (shouldShuffleKwLists) {
        for (auto& pair : ind) {
            Db<DbTuple>& dbKwList = pair.second;
            dbKwList.shuffle();
        }
    }

    return ind;
}


template <IsDbTuple DbTuple>
Range<typename DbTuple::DbKwType> findDbKwBounds(const Db<DbTuple>& db) {
    using DbKw = typename DbTuple::DbKwType;

    if (db.empty()) {
        return DUMMY_RANGE<DbKw>();
    }

    DbKw minDbKw = DUMMY;
    DbKw maxDbKw = DUMMY;
    for (DbTuple dbTuple : db) {
        Range<DbKw> dbKwRange = dbTuple.getDbKwRange();
        if (dbKwRange.first < minDbKw || minDbKw == DUMMY) {
            minDbKw = dbKwRange.first;
        }
        if (dbKwRange.second > maxDbKw || maxDbKw == DUMMY) {
            maxDbKw = dbKwRange.second;
        }
    }
    return Range {minDbKw, maxDbKw};
}


template <IsDbTuple DbTuple>
std::unordered_set<Range<typename DbTuple::DbKwType>> getUniqDbKwRanges(const Db<DbTuple>& db) {
    using DbKw = typename DbTuple::DbKwType;

    std::unordered_set<Range<DbKw>> uniqDbKwRanges;
    for (DbTuple dbTuple : db) {
        Range<DbKw> dbKwRange = dbTuple.getDbKwRange();
        uniqDbKwRanges.insert(dbKwRange); // `unordered_set` will not insert duplicate elements
    }
    return uniqDbKwRanges;
}


template <IsDbTuple DbTuple>
void padDb(Db<DbTuple>& db, typename DbTuple::DbKwType& currMaxDbKw) {
    using DbKw = typename DbTuple::DbKwType;

    int64_t dbSize = db.size();
    if (!std::has_single_bit((uint64_t)dbSize)) {
        int64_t amountToPad = std::pow(2, std::ceil(std::log2(dbSize))) - dbSize;
        for (int64_t i = 0; i < amountToPad; i++) {
            currMaxDbKw++;
            Range<DbKw> dbKwRange {currMaxDbKw, currMaxDbKw};
            DbTuple dummyTuple = DbTuple::genDummy(dbKwRange);
            db.push_back(dummyTuple);
        }
    }
}


// (we need the general case of this function to be able to call it from within the general context
// of `IStaticPointSse`; it just does nothing except in the template specialization below)
template <IsDbTuple DbTuple>
std::vector<DbTuple> cleanUpResults(const std::vector<DbTuple>& dbTuples) {
    return dbTuples;
}


// template specialize this method for just `Tuple<>` instead of all
// SSE classes that use it
template <>
std::vector<Tuple<>> cleanUpResults(const std::vector<Tuple<>>& tuples) {
    std::vector<Tuple<>> newTuples;
    std::unordered_set<Id> deletedIds;

    // find all cancellation tuples
    for (Tuple<> tuple : tuples) {
        Op op = tuple.getOp();
        if (op == Op::DEL) {
            Id id = tuple.getId();
            deletedIds.insert(id);
        }
    }
    // copy over vector without deleted (or dummy) tuples, as well as no dummy ids
    for (Tuple<> tuple : tuples) {
        Id id = tuple.getId();
        Op op = tuple.getOp();
        if (id != DUMMY && op == Op::INS && deletedIds.count(id) == 0) {
            newTuples.push_back(tuple);
        }
    }

    return newTuples;
}


uint64_t hashToPos(const ustring& hash) {
    // this conversion mess is from USENIX'24
    return (*((uint64_t*)hash.c_str()));
}


//------------------------------------------------------------------------------
// explicit template instantiations


template Ind<Tuple<>> genInd(const Db<Tuple<>>& db, bool shouldShuffleKwLists);
template Ind<SrcIDb1Tuple> genInd(const Db<SrcIDb1Tuple>& db, bool shouldShuffleKwLists);
//template Ind<Tuple<IdAlias>> genInd(
//    const Db<Tuple<IdAlias>>& db, bool shouldShuffleKwLists
//);


template Range<Kw> findDbKwBounds(const Db<Tuple<>>& db);
template Range<Kw> findDbKwBounds(const Db<SrcIDb1Tuple>& db);
//template Range<Kw> findDbKwBounds(const Db<Tuple<IdAlias>>& db);


template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<Tuple<>>& db);
template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<SrcIDb1Tuple>& db);
//template std::unordered_set<Range<IdAlias>> getUniqDbKwRanges(const Db<Tuple<IdAlias>>& db);


template void padDb(Db<Tuple<>>& db, Kw& currMaxDbKw);
template void padDb(Db<SrcIDb1Tuple>& db, Kw& currMaxDbKw);
//template void padDb(Db<Tuple<IdAlias>>& db, IdAlias& currMaxDbKw);


// remaining explicit template specializations beyond the one earlier
template std::vector<SrcIDb1Tuple> cleanUpResults(const std::vector<SrcIDb1Tuple>& tuples);
//template std::vector<Tuple<IdAlias>> cleanUpResults(const std::vector<Tuple<IdAlias>>& tuples);


} // namespace `utils`
