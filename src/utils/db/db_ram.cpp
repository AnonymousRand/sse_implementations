#include "utils/db_ram.h"

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
    this->vec.clear();
}


template <IsDbTuple DbTuple>
void DbRam<DbTuple>::push_back(const DbTuple& dbTuple) {
    this->vec.push_back(dbTuple);
}


template <IsDbTuple DbTuple>
bigint DbRam<DbTuple>::size() const {
    return this->vec.size();
}


template <IsDbTuple DbTuple>
bool DbRam<DbTuple>::empty() const {
    return this->vec.empty();
}


template <IsDbTuple DbTuple>
DbTuple DbRam<DbTuple>::operator [](bigint index) const {
    return this->vec[index];
}


template <IsDbTuple DbTuple>
void DbRam<DbTuple>::shuffle() {
    std::shuffle(this->begin(), this->end(), utils::RNG);
}


template <IsDbTuple DbTuple>
void DbRam<DbTuple>::sort(const std::function<bool(bigint index1, bigint index2)>& compare) {
    std::sort(this->begin(), this->end(), compare);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class DbRam<Tuple<>>;
template class DbRam<SrcIDb1Tuple>;
//template class DbRam<Tuple<IdAlias>>;
