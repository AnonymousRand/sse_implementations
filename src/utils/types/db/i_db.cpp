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
    this->minDbKw = DUMMY;
    this->maxDbKw = DUMMY;
}


template <IsDbTuple DbTuple>
std::unordered_set<Range<typename DbTuple::DbKwType>> IDb<DbTuple>::getUniqDbKwRanges() const {
    std::unordered_set<Range<DbKw>> uniqDbKwRanges;
    for (const DbTuple& dbTuple : *this) {
        // (`unordered_set` will not insert duplicate elements)
        uniqDbKwRanges.emplace(dbTuple.getDbKwRange());
    }
    return uniqDbKwRanges;
}


template <IsDbTuple DbTuple>
void IDb<DbTuple>::padToPowOf2() {
    bigint dbSize = this->_size;
    if (!std::has_single_bit((ubigint)dbSize)) {
        bigint amountToPad = std::pow(2, std::ceil(std::log2(dbSize))) - dbSize;
        this->reserve(this->_size + amountToPad);
        for (bigint i = 0; i < amountToPad; i++) {
            Range<DbKw> dbKwRange {this->maxDbKw + 1, this->maxDbKw + 1};
            DbTuple dummyTuple = DbTuple::DUMMY(dbKwRange);
            this->push_back(dummyTuple);
        }
    }
}


template <IsDbTuple DbTuple>
void IDb<DbTuple>::onNewDbTuple(const DbTuple& dbTuple) {
    this->_size++;

    Range<DbKw> dbKwRange = dbTuple.getDbKwRange();
    if (dbKwRange.first < this->minDbKw || this->minDbKw == DUMMY) {
        this->minDbKw = dbKwRange.first;
    }
    if (dbKwRange.second > this->maxDbKw || this->maxDbKw == DUMMY) {
        this->maxDbKw = dbKwRange.second;
    }
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class IDb<Tuple<>>;        // default/input DBs
template class IDb<SrcIDb1Tuple>;   // Log-SRC-i index 1 DBs
//template class IDb<Tuple<IdAlias>>; // Log-SRC-i index 2 DBs (commented out as `IdAlias` = `Kw`)
