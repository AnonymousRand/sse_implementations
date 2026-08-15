#include "utils/ind.h"


//------------------------------------------------------------------------------
// constructors/destructors


template <IsDbTuple DbTuple>
Ind<DbTuple>::Ind(const Db<DbTuple>& db, bool shouldShuffleKwLists) {
    for (DbTuple dbTuple : db) {
        Range<DbKw> dbKwRange = dbTuple.getDbKwRange();
        if (this->count(dbKwRange) == 0) {
            this[dbKwRange] = Db<DbTuple> {dbTuple};
        } else {
            this[dbKwRange].push_back(dbTuple);
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
ConstIter Ind<DbTuple>::find(const KeyType& key) const {
    return this->map.find(key);
}


template <IsDbTuple DbTuple>
int64_t Ind<DbTuple>::count(const KeyType& key) const {
    return this->map.count(key);
}


template <IsDbTuple DbTuple>
ValType& operator [](const KeyType& key) {
    return this->map[key];
}


template <IsDbTuple DbTuple>
const ValType& operator [](const KeyType& key) const {
    return this->map.at(key);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class Ind<Tuple<>>;
template class Ind<SrcIDb1Doc>;
//template class Ind<Tuple<IdAlias>>;
