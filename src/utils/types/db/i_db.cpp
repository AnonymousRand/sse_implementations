#include "utils/types/db/i_db.h"

#include <bit>
#include <cmath>
#include <concepts>
#include <unordered_set>

#include "utils/types/basic_types.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


//------------------------------------------------------------------------------
// interface


template <IsDbTuple DbTuple>
void IDb<DbTuple>::clear() {
    this->_size = 0;
}


//------------------------------------------------------------------------------
// utils


template <IsDbTuple DbTuple>
Range<typename DbTuple::DbKwType> IDb<DbTuple>::findDbKwBounds() const {
    using DbKw = typename DbTuple::DbKwType;

    if (this->empty()) {
        return Range<DbKw>::DUMMY();
    }

    DbKw minDbKw = DUMMY;
    DbKw maxDbKw = DUMMY;
    for (const DbTuple& dbTuple : *this) {
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
    for (const DbTuple& dbTuple : *this) {
        Range<DbKw> dbKwRange = dbTuple.getDbKwRange();
        uniqDbKwRanges.insert(dbKwRange); // `unordered_set` will not insert duplicate elements
    }
    return uniqDbKwRanges;
}


template <IsDbTuple DbTuple>
void IDb<DbTuple>::pad(typename DbTuple::DbKwType& currMaxDbKw) {
    using DbKw = typename DbTuple::DbKwType;

    bigint dbSize = this->_size;
    if (!std::has_single_bit((ubigint)dbSize)) {
        bigint amountToPad = std::pow(2, std::ceil(std::log2(dbSize))) - dbSize;
        this->reserve(this->_size + amountToPad);
        for (bigint i = 0; i < amountToPad; i++) {
            currMaxDbKw++;
            Range<DbKw> dbKwRange {currMaxDbKw, currMaxDbKw};
            DbTuple dummyTuple = DbTuple::DUMMY(dbKwRange);
            this->push_back(dummyTuple);
        }
    }
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class IDb<Tuple<>>;        // default/input DBs
template class IDb<SrcIDb1Tuple>;   // Log-SRC-i index 1 DBs
//template class IDb<Tuple<IdAlias>>; // Log-SRC-i index 2 DBs (commented out as `IdAlias` = `Kw`)
