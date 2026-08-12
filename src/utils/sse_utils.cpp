#include "sse_utils.h"

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <unordered_set>
#include <utility>
#include <vector>

#include "utils/db.h"
#include "utils/random.h"
#include "utils/range.h"
#include "utils/ustring.h"


namespace utils {


template <class DbTuple, class DbKw> requires IsValidDbParams<DbTuple, DbKw>
Ind<DbKw, DbTuple> genInd(const Db<DbTuple, DbKw>& db, bool shouldShuffleKwLists) {
    Ind<DbKw, DbTuple> ind;
    for (DbTuple<DbTuple, DbKw> entry : db) {
        DbTuple dbTuple = entry.first;
        Range<DbKw> dbKwRange = entry.second;
        if (ind.count(dbKwRange) == 0) {
            ind[dbKwRange] = std::vector {dbTuple};
        } else {
            ind[dbKwRange].push_back(dbTuple);
        }
    }

    if (shouldShuffleKwLists) {
        for (std::pair entry : ind) {
            std::vector<DbTuple> dbKwList = entry.second;
            std::shuffle(dbKwList.begin(), dbKwList.end(), RNG);
        }
    }

    return ind;
}


template <class DbTuple, class DbKw> requires IsValidDbParams<DbTuple, DbKw>
Range<DbKw> findDbKwBounds(const Db<DbTuple, DbKw>& db) {
    if (db.empty()) {
        return DUMMY_RANGE<DbKw>();
    }

    DbKw minDbKw = DUMMY;
    DbKw maxDbKw = DUMMY;
    for (DbTuple<DbTuple, DbKw> entry : db) {
        Range<DbKw> dbKwRange = entry.second;
        if (dbKwRange.first < minDbKw || minDbKw == DUMMY) {
            minDbKw = dbKwRange.first;
        }
        if (dbKwRange.second > maxDbKw || maxDbKw == DUMMY) {
            maxDbKw = dbKwRange.second;
        }
    }
    return Range {minDbKw, maxDbKw};
}


template <class DbTuple, class DbKw> requires IsValidDbParams<DbTuple, DbKw>
std::unordered_set<Range<DbKw>> getUniqDbKwRanges(const Db<DbTuple, DbKw>& db) {
    std::unordered_set<Range<DbKw>> uniqDbKwRanges;
    for (DbTuple<DbTuple, DbKw> entry : db) {
        Range<DbKw> dbKwRange = entry.second;
        uniqDbKwRanges.insert(dbKwRange); // `unordered_set` will not insert duplicate elements
    }
    return uniqDbKwRanges;
}


// (note we still need the general case of this function to be able to call it from within
// `IStaticPointSse`; it just does nothing except in the template specialization below)
template <IsDbTuple DbTuple>
void cleanUpResults(std::vector<DbTuple>& dbTuples) {}


// template specialize this method for just `Tuple<>` instead of all
// SSE classes that use it
template <>
void cleanUpResults(std::vector<Tuple<>>& dbTuples) {
    std::vector<Tuple<>> newDbTuples;
    std::unordered_set<Id> deletedIds;

    // find all cancellation tuples
    for (Tuple<> dbTuple : dbTuples) {
        Op op = dbTuple.getOp();
        if (op == Op::DEL) {
            Id id = dbTuple.getId();
            deletedIds.insert(id);
        }
    }
    // copy over vector without deleted (or dummy) dbTuples, as well as no dummy ids
    for (Tuple<> dbTuple : dbTuples) {
        Id id = dbTuple.getId();
        Op op = dbTuple.getOp();
        if (id != DUMMY && op == Op::INS && deletedIds.count(id) == 0) {
            newDbTuples.push_back(dbTuple);
        }
    }

    dbTuples = newDbTuples;
}


uint64_t hashToPos(const ustring& hash) {
    // this conversion mess is from USENIX'24
    return (*((uint64_t*)hash.c_str()));
}


//------------------------------------------------------------------------------
// explicit template instantiations


template Ind<Kw, Tuple<>> genInd(
    const Db<Tuple<>, Kw>& db, bool shouldShuffleKwLists
);
template Ind<Kw, SrcIDb1Tuple> genInd(
    const Db<SrcIDb1Tuple, Kw>& db, bool shouldShuffleKwLists
);
//template Ind<IdAlias, Tuple<IdAlias>> genInd(
//    const Db<Tuple<IdAlias>, IdAlias>& db, bool shouldShuffleKwLists
//);

template Range<Kw> findDbKwBounds(const Db<Tuple<>, Kw>& db);
template Range<Kw> findDbKwBounds(const Db<SrcIDb1Tuple, Kw>& db);
//template Range<Kw> findDbKwBounds(const Db<Tuple<IdAlias>, IdAlias>& db);

template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<Tuple<>, Kw>& db);
template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<SrcIDb1Tuple, Kw>& db);
//template std::unordered_set<Range<IdAlias>> getUniqDbKwRanges(
//    const Db<Tuple<IdAlias>, IdAlias>& db
//);

// remaining explicit template specializations beyond the one previously
template void cleanUpResults(std::vector<SrcIDb1Tuple>& dbTuples);
//template void cleanUpResults(std::vector<Tuple<IdAlias>>& dbTuples);


} // namespace `utils`
