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
#include "utils/types.h"
#include "utils/ustring.h"


namespace utils {


template <class DbTuple, class DbKw> requires IsValidDbParams<DbTuple, DbKw>
Ind<DbKw, DbTuple> genInd(const Db<DbTuple>& db, bool shouldShuffleKwLists) {
    Ind<DbKw, DbTuple> ind;
    for (DbTuple dbTuple : db) {
        Range<DbKw> dbKwRange = dbTuple.getDbKwRange();
        if (ind.count(dbKwRange) == 0) {
            ind[dbKwRange] = std::vector {dbTuple};
        } else {
            ind[dbKwRange].push_back(dbTuple);
        }
    }

    if (shouldShuffleKwLists) {
        for (std::pair pair : ind) {
            std::vector<DbTuple> dbKwList = pair.second;
            std::shuffle(dbKwList.begin(), dbKwList.end(), RNG);
        }
    }

    return ind;
}


template <class DbTuple, class DbKw> requires IsValidDbParams<DbTuple, DbKw>
Range<DbKw> findDbKwBounds(const Db<DbTuple>& db) {
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


template <class DbTuple, class DbKw> requires IsValidDbParams<DbTuple, DbKw>
std::unordered_set<Range<DbKw>> getUniqDbKwRanges(const Db<DbTuple>& db) {
    std::unordered_set<Range<DbKw>> uniqDbKwRanges;
    for (DbTuple dbTuple : db) {
        Range<DbKw> dbKwRange = dbTuple.getDbKwRange();
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
void cleanUpResults(std::vector<Tuple<>>& tuples) {
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

    tuples = newTuples;
}


uint64_t hashToPos(const ustring& hash) {
    // this conversion mess is from USENIX'24
    return (*((uint64_t*)hash.c_str()));
}


//------------------------------------------------------------------------------
// explicit template instantiations


template Ind<Kw, Tuple<>> genInd(const Db<Tuple<>>& db, bool shouldShuffleKwLists);
template Ind<Kw, SrcIDb1Tuple> genInd(const Db<SrcIDb1Tuple>& db, bool shouldShuffleKwLists);
//template Ind<IdAlias, Tuple<IdAlias>> genInd(
//    const Db<Tuple<IdAlias>>& db, bool shouldShuffleKwLists
//);

template Range<Kw> findDbKwBounds(const Db<Tuple<>>& db);
template Range<Kw> findDbKwBounds(const Db<SrcIDb1Tuple>& db);
//template Range<Kw> findDbKwBounds(const Db<Tuple<IdAlias>>& db);

template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<Tuple<>>& db);
template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<SrcIDb1Tuple>& db);
//template std::unordered_set<Range<IdAlias>> getUniqDbKwRanges(const Db<Tuple<IdAlias>>& db);

// remaining explicit template specializations beyond the one earlier
template void cleanUpResults(std::vector<SrcIDb1Tuple>& tuples);
//template void cleanUpResults(std::vector<Tuple<IdAlias>>& tuples);


} // namespace `utils`
