#include "utils/db/db_ram.h"

#include <algorithm>
#include <concepts>
#include <functional>
#include <initializer_list>
#include <vector>

#include "utils/random.h"
#include "utils/tuple.h"
#include "utils/types.h"


//------------------------------------------------------------------------------
// constructors/destructors


template <IsDbTuple DbTuple>
DbRam<DbTuple>::DbRam(const DbRam& db, bigint startIndex, bigint endIndex) :
    vec(db.vec.begin() + startIndex, db.vec.end() + endIndex) {}


template <IsDbTuple DbTuple>
DbRam<DbTuple>::DbRam(std::initializer_list<DbTuple> initList) : vec(initList) {}


//------------------------------------------------------------------------------
// `IDb`


template <IsDbTuple DbTuple>
void DbRam<DbTuple>::clear() {
    // clears `this->_size`
    IDb<DbTuple>::clear();

    // clears DB vector
    this->vec.clear();
}


template <IsDbTuple DbTuple>
void DbRam<DbTuple>::push_back(const DbTuple& dbTuple) {
    this->vec.push_back(dbTuple);
}


template <IsDbTuple DbTuple>
DbTuple DbRam<DbTuple>::operator [](bigint index) const {
    return this->vec[index];
}


template <IsDbTuple DbTuple>
void DbRam<DbTuple>::shuffle() {
    // (note: not using `this->begin()` and `this->end()` here to avoid needing to make
    // `IDb::Iter` a fully fledged `LegacyRandomAccessIterator`)
    std::shuffle(this->vec.begin(), this->vec.end(), utils::RNG);
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
