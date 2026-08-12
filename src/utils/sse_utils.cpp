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


template <class DbEntry, class DbKw> requires IsValidDbParams<DbEntry, DbKw>
Ind<DbKw, DbEntry> genInd(const Db<DbEntry, DbKw>& db, bool shouldShuffleKwLists) {
    Ind<DbKw, DbEntry> ind;
    for (DbEntry<DbEntry, DbKw> entry : db) {
        DbEntry dbEntry = entry.first;
        Range<DbKw> dbKwRange = entry.second;
        if (ind.count(dbKwRange) == 0) {
            ind[dbKwRange] = std::vector {dbEntry};
        } else {
            ind[dbKwRange].push_back(dbEntry);
        }
    }

    if (shouldShuffleKwLists) {
        for (std::pair entry : ind) {
            std::vector<DbEntry> dbKwList = entry.second;
            std::shuffle(dbKwList.begin(), dbKwList.end(), RNG);
        }
    }

    return ind;
}


template <class DbEntry, class DbKw> requires IsValidDbParams<DbEntry, DbKw>
Range<DbKw> findDbKwBounds(const Db<DbEntry, DbKw>& db) {
    if (db.empty()) {
        return DUMMY_RANGE<DbKw>();
    }

    DbKw minDbKw = DUMMY;
    DbKw maxDbKw = DUMMY;
    for (DbEntry<DbEntry, DbKw> entry : db) {
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


template <class DbEntry, class DbKw> requires IsValidDbParams<DbEntry, DbKw>
std::unordered_set<Range<DbKw>> getUniqDbKwRanges(const Db<DbEntry, DbKw>& db) {
    std::unordered_set<Range<DbKw>> uniqDbKwRanges;
    for (DbEntry<DbEntry, DbKw> entry : db) {
        Range<DbKw> dbKwRange = entry.second;
        uniqDbKwRanges.insert(dbKwRange); // `unordered_set` will not insert duplicate elements
    }
    return uniqDbKwRanges;
}


// (note we still need the general case of this function to be able to call it from within
// `IStaticPointSse`; it just does nothing except in the template specialization below)
template <IsDbEntry DbEntry>
void cleanUpResults(std::vector<DbEntry>& dbEntries) {}


// template specialize this method for just `DefaultDbEntry<>` instead of all
// SSE classes that use it
template <>
void cleanUpResults(std::vector<DefaultDbEntry<>>& dbEntries) {
    std::vector<DefaultDbEntry<>> newDbEntries;
    std::unordered_set<Id> deletedIds;

    // find all cancellation tuples
    for (DefaultDbEntry<> dbEntry : dbEntries) {
        Op op = dbEntry.getOp();
        if (op == Op::DEL) {
            Id id = dbEntry.getId();
            deletedIds.insert(id);
        }
    }
    // copy over vector without deleted (or dummy) dbEntries, as well as no dummy ids
    for (DefaultDbEntry<> dbEntry : dbEntries) {
        Id id = dbEntry.getId();
        Op op = dbEntry.getOp();
        if (id != DUMMY && op == Op::INS && deletedIds.count(id) == 0) {
            newDbEntries.push_back(dbEntry);
        }
    }

    dbEntries = newDbEntries;
}


uint64_t hashToPos(const ustring& hash) {
    // this conversion mess is from USENIX'24
    return (*((uint64_t*)hash.c_str()));
}


//------------------------------------------------------------------------------
// explicit template instantiations


template Ind<Kw, DefaultDbEntry<>> genInd(
    const Db<DefaultDbEntry<>, Kw>& db, bool shouldShuffleKwLists
);
template Ind<Kw, SrcIDb1Entry> genInd(
    const Db<SrcIDb1Entry, Kw>& db, bool shouldShuffleKwLists
);
//template Ind<IdAlias, DefaultDbEntry<IdAlias>> genInd(
//    const Db<DefaultDbEntry<IdAlias>, IdAlias>& db, bool shouldShuffleKwLists
//);

template Range<Kw> findDbKwBounds(const Db<DefaultDbEntry<>, Kw>& db);
template Range<Kw> findDbKwBounds(const Db<SrcIDb1Entry, Kw>& db);
//template Range<Kw> findDbKwBounds(const Db<DefaultDbEntry<IdAlias>, IdAlias>& db);

template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<DefaultDbEntry<>, Kw>& db);
template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<SrcIDb1Entry, Kw>& db);
//template std::unordered_set<Range<IdAlias>> getUniqDbKwRanges(
//    const Db<DefaultDbEntry<IdAlias>, IdAlias>& db
//);

// remaining explicit template specializations beyond the one previously
template void cleanUpResults(std::vector<SrcIDb1Entry>& dbEntries);
//template void cleanUpResults(std::vector<DefaultDbEntry<IdAlias>>& dbEntries);


} // namespace `utils`
