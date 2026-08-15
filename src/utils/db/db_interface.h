#pragma once

#include <concepts>
#include <functional>
#include <unordered_set>

#include "utils/range.h"
#include "utils/tuple.h"
#include "utils/types.h"


template <IsDbTuple DbTuple = Tuple<>>
class IDb {
public:
    // note: currently, iterator `.begin()` and `.end()` are not enforced here because i
    // don't know if there's an easy way to do that (given the different types of iterators
    // possible; and since custom iterators are declared as nested classes of `IDb` classes,
    // passing this type as a template param might not work)
    // 
    // constructors are also not enforced since you can't really make them virtual/pure virtual

    //--------------------------------------------------------------------------
    // methods to implement

    virtual void clear() = 0;
    virtual void push_back(const DbTuple& dbTuple) = 0;
    virtual bigint size() const = 0;
    virtual bool empty() const = 0;

    virtual DbTuple operator [](bigint index) const = 0;

    virtual void shuffle() = 0;
    virtual void sort(const std::function<bool(bigint index1, bigint index2)>& compare) = 0;

    //--------------------------------------------------------------------------
    // utils

    Range<typename DbTuple::DbKwType> findDbKwBounds() const;
    std::unordered_set<Range<typename DbTuple::DbKwType>> getUniqDbKwRanges() const;
    void pad(typename DbTuple::DbKwType& currMaxDbKw);
};
