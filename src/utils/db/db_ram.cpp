#include "utils/db_ram.h"

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <vector>

#include "utils/random.h"
#include "utils/tuple.h"


//------------------------------------------------------------------------------
// constructors/destructors


template <IsDbTuple DbTuple>
DbRam<DbTuple>::DbRam(const DbRam& db, int64_t startIndex, int64_t endIndex) :
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
int64_t DbRam<DbTuple>::size() const {
    return this->vec.size();
}


template <IsDbTuple DbTuple>
bool DbRam<DbTuple>::empty() const {
    return this->vec.empty();
}


template <IsDbTuple DbTuple>
DbTuple DbRam<DbTuple>::operator [](int64_t index) const {
    return this->vec[index];
}


template <IsDbTuple DbTuple>
void DbRam<DbTuple>::shuffle() {
    std::shuffle(this->begin(), this->end(), utils::RNG);
}


template <IsDbTuple DbTuple>
void DbRam<DbTuple>::sort(const std::function<bool(int64_t index1, int64_t index2)>& compare) {
    std::sort(this->begin(), this->end(), compare);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class DbRam<Tuple<>>;        // default/input DBs
template class DbRam<SrcIDb1Tuple>;   // Log-SRC-i index 1 DBs
//template class DbRam<Tuple<IdAlias>>; // Log-SRC-i index 2 DBs
