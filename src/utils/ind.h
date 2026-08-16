#pragma once

#include <concepts>
#include <unordered_map>

#include "utils/db/db.h"
#include "utils/range.h"
#include "utils/tuple.h"
#include "utils/types.h"


template <IsDbTuple DbTuple = Tuple<>>
class Ind {
private:
    using DbKw      = typename DbTuple::DbKwType;
    using KeyType   = Range<DbKw>;
    using ValType   = Db<DbTuple>;
    using InnerType = std::unordered_map<KeyType, ValType>;
    using Iter      = InnerType::iterator;
    using ConstIter = InnerType::const_iterator;

public:
    //--------------------------------------------------------------------------
    // constructors/destructors

    Ind(const Db<DbTuple>& db, bool shouldShuffleKwLists = false);

    //--------------------------------------------------------------------------
    // interface

    Iter find(const KeyType& key);
    ConstIter find(const KeyType& key) const;
    bigint count(const KeyType& key) const;

    ValType& operator [](const KeyType& key);
    const ValType& operator [](const KeyType& key) const;

    //--------------------------------------------------------------------------
    // utils

private:
    InnerType map;

//------------------------------------------------------------------------------
// iterator

public:
    // non-const iterators
    Iter begin() {
        return this->map.begin();
    }

    Iter end() {
        return this->map.end();
    }

    // const iterators
    ConstIter begin() const {
        return this->map.begin();
    }

    ConstIter end() const {
        return this->map.end();
    }
};
