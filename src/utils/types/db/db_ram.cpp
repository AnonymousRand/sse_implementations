#include "utils/types/db/db_ram.h"

#include <algorithm>
#include <concepts>
#include <functional>
#include <initializer_list>
#include <vector>

#include "utils/random.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/i_db.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


//------------------------------------------------------------------------------
// constructors/destructors


template <IsDbTuple DbTuple>
DbRam<DbTuple>::DbRam(const DbRam& other, bigint startIndex, bigint endIndex) {
    // doing this manually in order to be able to set `minDbKw` and `maxDbKw` members
    this->vec.reserve(endIndex - startIndex);
    for (bigint index = startIndex; index < endIndex; index++) {
        DbTuple dbTuple = other[index];
        this->push_back(dbTuple);
    }
}


template <IsDbTuple DbTuple>
DbRam<DbTuple>::DbRam(std::initializer_list<DbTuple> initList) {
    this->vec.reserve(initList.size());
    for (const DbTuple& dbTuple : initList) {
        this->push_back(dbTuple);
    }
}


//------------------------------------------------------------------------------
// `IDb`


template <IsDbTuple DbTuple>
void DbRam<DbTuple>::clear() {
    // clears DB vector
    this->vec.clear();

    // clears `this->_size`
    IDb<DbTuple>::clear();
}


template <IsDbTuple DbTuple>
void DbRam<DbTuple>::push_back(const DbTuple& dbTuple) {
    this->vec.push_back(dbTuple);

    // update member variables as needed
    this->onNewDbTuple(dbTuple);
}


template <IsDbTuple DbTuple>
DbTuple DbRam<DbTuple>::operator [](bigint index) const {
    return this->vec[index];
}


template <IsDbTuple DbTuple>
void DbRam<DbTuple>::reserve(bigint size) {
    this->vec.reserve(size);
}


template <IsDbTuple DbTuple>
void DbRam<DbTuple>::shuffle() {
    // (note: not using `this->begin()` and `this->end()` here to avoid needing to make
    // `IDb::Iter` a fully fledged `LegacyRandomAccessIterator`)
    std::shuffle(this->vec.begin(), this->vec.end(), utils::random::RNG);
}


template <IsDbTuple DbTuple>
void DbRam<DbTuple>::sort(
    const std::function<bool(const DbTuple& dbTuple1, const DbTuple& dbTuple2)>& compare
) {
    std::sort(this->vec.begin(), this->vec.end(), compare);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class DbRam<Tuple<>>;
template class DbRam<SrcIDb1Tuple>;
//template class DbRam<Tuple<IdAlias>>;
