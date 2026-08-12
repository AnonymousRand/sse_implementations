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


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
Ind<DbKw, DbRecord> genInd(const Db<DbRecord, DbKw>& db, bool shouldShuffleKwLists) {
    Ind<DbKw, DbRecord> ind;
    for (DbRecord<DbRecord, DbKw> entry : db) {
        DbRecord dbRecord = entry.first;
        Range<DbKw> dbKwRange = entry.second;
        if (ind.count(dbKwRange) == 0) {
            ind[dbKwRange] = std::vector {dbRecord};
        } else {
            ind[dbKwRange].push_back(dbRecord);
        }
    }

    if (shouldShuffleKwLists) {
        for (std::pair entry : ind) {
            std::vector<DbRecord> dbKwList = entry.second;
            std::shuffle(dbKwList.begin(), dbKwList.end(), RNG);
        }
    }

    return ind;
}


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
Range<DbKw> findDbKwBounds(const Db<DbRecord, DbKw>& db) {
    if (db.empty()) {
        return DUMMY_RANGE<DbKw>();
    }

    DbKw minDbKw = DUMMY;
    DbKw maxDbKw = DUMMY;
    for (DbRecord<DbRecord, DbKw> entry : db) {
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


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
std::unordered_set<Range<DbKw>> getUniqDbKwRanges(const Db<DbRecord, DbKw>& db) {
    std::unordered_set<Range<DbKw>> uniqDbKwRanges;
    for (DbRecord<DbRecord, DbKw> entry : db) {
        Range<DbKw> dbKwRange = entry.second;
        uniqDbKwRanges.insert(dbKwRange); // `unordered_set` will not insert duplicate elements
    }
    return uniqDbKwRanges;
}


// (note we still need the general case of this function to be able to call it from within
// `IStaticPointSse`; it just does nothing except in the template specialization below)
template <IsDbRecord DbRecord>
void cleanUpResults(std::vector<DbRecord>& dbRecords) {}


// template specialize this method for just `Record<>` instead of all
// SSE classes that use it
template <>
void cleanUpResults(std::vector<Record<>>& dbRecords) {
    std::vector<Record<>> newDbRecords;
    std::unordered_set<Id> deletedIds;

    // find all cancellation tuples
    for (Record<> dbRecord : dbRecords) {
        Op op = dbRecord.getOp();
        if (op == Op::DEL) {
            Id id = dbRecord.getId();
            deletedIds.insert(id);
        }
    }
    // copy over vector without deleted (or dummy) dbRecords, as well as no dummy ids
    for (Record<> dbRecord : dbRecords) {
        Id id = dbRecord.getId();
        Op op = dbRecord.getOp();
        if (id != DUMMY && op == Op::INS && deletedIds.count(id) == 0) {
            newDbRecords.push_back(dbRecord);
        }
    }

    dbRecords = newDbRecords;
}


uint64_t hashToPos(const ustring& hash) {
    // this conversion mess is from USENIX'24
    return (*((uint64_t*)hash.c_str()));
}


//------------------------------------------------------------------------------
// explicit template instantiations


template Ind<Kw, Record<>> genInd(
    const Db<Record<>, Kw>& db, bool shouldShuffleKwLists
);
template Ind<Kw, SrcIDb1Record> genInd(
    const Db<SrcIDb1Record, Kw>& db, bool shouldShuffleKwLists
);
//template Ind<IdAlias, Record<IdAlias>> genInd(
//    const Db<Record<IdAlias>, IdAlias>& db, bool shouldShuffleKwLists
//);

template Range<Kw> findDbKwBounds(const Db<Record<>, Kw>& db);
template Range<Kw> findDbKwBounds(const Db<SrcIDb1Record, Kw>& db);
//template Range<Kw> findDbKwBounds(const Db<Record<IdAlias>, IdAlias>& db);

template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<Record<>, Kw>& db);
template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<SrcIDb1Record, Kw>& db);
//template std::unordered_set<Range<IdAlias>> getUniqDbKwRanges(
//    const Db<Record<IdAlias>, IdAlias>& db
//);

// remaining explicit template specializations beyond the one previously
template void cleanUpResults(std::vector<SrcIDb1Record>& dbRecords);
//template void cleanUpResults(std::vector<Record<IdAlias>>& dbRecords);


} // namespace `utils`
