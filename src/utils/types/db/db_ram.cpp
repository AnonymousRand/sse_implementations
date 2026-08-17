#include "utils/types/db/db_ram.h"

#include <algorithm>
#include <concepts>
#include <functional>
#include <initializer_list>
#include <vector>

#include "utils/random.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/i_db.h"
#include "utils/types/tuple.h"


//------------------------------------------------------------------------------
// constructors/destructors


template <IsDbTuple DbTuple>
DbRam<DbTuple>::DbRam(const DbRam& db, bigint startIndex, bigint endIndex) :
    vec(db.vec.begin() + startIndex, db.vec.begin() + endIndex)
{
   this->_size = this->vec.size();
}


template <IsDbTuple DbTuple>
DbRam<DbTuple>::DbRam(std::initializer_list<DbTuple> initList) :
    vec(initList)
{
    this->_size = this->vec.size();
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
    this->_size++;
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
