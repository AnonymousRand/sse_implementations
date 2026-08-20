#include "utils/types/ind.h"

#include <concepts>
#include <unordered_map>

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


//------------------------------------------------------------------------------
// constructors/destructors


template <IsDbTuple DbTuple>
Ind<DbTuple>::Ind(const Db<DbTuple>& db, bool shouldShuffleKwLists) {
    for (const DbTuple& dbTuple : db) {
        Range<DbKw> dbKwRange = dbTuple.getDbKwRange();
        if (!this->contains(dbKwRange)) {
            (*this)[dbKwRange] = Db<DbTuple> {dbTuple};
        } else {
            (*this)[dbKwRange].push_back(dbTuple);
        }
    }

    if (shouldShuffleKwLists) {
        for (auto& pair : *this) {
            Db<DbTuple>& dbKwList = pair.second;
            dbKwList.shuffle();
        }
    }
}


//------------------------------------------------------------------------------
// interface


template <IsDbTuple DbTuple>
Ind<DbTuple>::Iter Ind<DbTuple>::find(const KeyType& key) {
    return this->map.find(key);
}


template <IsDbTuple DbTuple>
Ind<DbTuple>::ConstIter Ind<DbTuple>::find(const KeyType& key) const {
    return this->map.find(key);
}


template <IsDbTuple DbTuple>
bool Ind<DbTuple>::contains(const KeyType& key) const {
    return this->map.contains(key);
}


template <IsDbTuple DbTuple>
Ind<DbTuple>::ValType& Ind<DbTuple>::operator [](const KeyType& key) {
    return this->map[key];
}


template <IsDbTuple DbTuple>
const Ind<DbTuple>::ValType& Ind<DbTuple>::operator [](const KeyType& key) const {
    return this->map.at(key);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class Ind<Tuple<>>;
template class Ind<SrcIDb1Tuple>;
//template class Ind<Tuple<IdAlias>>;
