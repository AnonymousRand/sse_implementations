#include "utils/db/db_interface.h"

#include <cmath>
#include <concepts>
#include <cstdint>
#include <unordered_set>

#include "utils/range.h"
#include "utils/tuple.h"


//------------------------------------------------------------------------------
// utils


template <IsDbTuple DbTuple>
Range<typename DbTuple::DbKwType> IDb<DbTuple>::findDbKwBounds() const {
    using DbKw = typename DbTuple::DbKwType;

    if (this->empty()) {
        return DUMMY_RANGE<DbKw>();
    }

    DbKw minDbKw = DUMMY;
    DbKw maxDbKw = DUMMY;
    for (DbTuple dbTuple : *this) {
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
std::unordered_set<Range<typename DbTuple::DbKwType>> IDb<DbTuple>::getUniqDbKwRanges() const {
    using DbKw = typename DbTuple::DbKwType;

    std::unordered_set<Range<DbKw>> uniqDbKwRanges;
    for (DbTuple dbTuple : *this) {
        Range<DbKw> dbKwRange = dbTuple.getDbKwRange();
        uniqDbKwRanges.insert(dbKwRange); // `unordered_set` will not insert duplicate elements
    }
    return uniqDbKwRanges;
}


template <IsDbTuple DbTuple>
void IDb<DbTuple>::pad(typename DbTuple::DbKwType& currMaxDbKw) {
    using DbKw = typename DbTuple::DbKwType;

    int64_t dbSize = this->size();
    if (!std::has_single_bit((uint64_t)dbSize)) {
        int64_t amountToPad = std::pow(2, std::ceil(std::log2(dbSize))) - dbSize;
        for (int64_t i = 0; i < amountToPad; i++) {
            currMaxDbKw++;
            Range<DbKw> dbKwRange {currMaxDbKw, currMaxDbKw};
            DbTuple dummyTuple = DbTuple::genDummy(dbKwRange);
            this->push_back(dummyTuple);
        }
    }
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class IDb<Tuple<>>;        // default/input DBs
template class IDb<SrcIDb1Tuple>;   // Log-SRC-i index 1 DBs
//template class IDb<Tuple<IdAlias>>; // Log-SRC-i index 2 DBs
