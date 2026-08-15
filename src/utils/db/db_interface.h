#pragma once

#include <concepts>
#include <functional>
#include <unordered_set>

#include "utils/range.h"
#include "utils/tuple.h"
#include "utils/types.h"


// implementations of this interface should essentially be `std::vector`s
// (hence we also try to match `std::vector`'s method names as much as possible here)
template <IsDbTuple DbTuple = Tuple<>>
class IDb {
public:
    //--------------------------------------------------------------------------
    // interface

    virtual void clear();
    virtual void push_back(const DbTuple& dbTuple) = 0;

    bigint size() const { return this->_size; }
    bool empty() const { return this->_size == 0; }

    // read-only accessing using `[]`
    // >TODO test what if this is const DbTuple&? same with Ind. maybe just implement both const
    // and non-const reference versions?
    virtual DbTuple operator [](bigint index) const = 0;

    virtual void shuffle() = 0;
    virtual void sort(const std::function<bool(bigint index1, bigint index2)>& compare) = 0;

    //--------------------------------------------------------------------------
    // utils

    Range<typename DbTuple::DbKwType> findDbKwBounds() const;
    std::unordered_set<Range<typename DbTuple::DbKwType>> getUniqDbKwRanges() const;
    void pad(typename DbTuple::DbKwType& currMaxDbKw);

protected:
    bigint _size = 0;

//------------------------------------------------------------------------------
// iterator

// this allows us to iterate through a `DbDisk` using range-based `for` loop or iterators,
// implementing the bare minimum operator overloads necessary to do so
// 
// (note this is not a `LegacyRandomAccessIterator`, meaning things like `std::sort()`
// which need to modify it in place will not work. this is because that requires dereferencing
// the iterator to return a reference to a `DbTuple` object, but `DbTuple` objects are only
// constructed on the fly when reading from a `DbDisk`, so this is basically impossible.)

public:
    // the template allows `DbTuple2` to be either `DbTuple` or `const DbTuple` so we can have both
    // non-const (e.g. for `std::sort()`) and const iterators (e.g. for iterating a `const DbDisk<>&`)
    template <class DbTuple2> requires std::is_same_v<std::remove_cv_t<DbTuple2>, DbTuple>
    struct Iter {
        const IDb<DbTuple>* db = nullptr;
        bigint index;

        Iter(const IDb<DbTuple>* db, bigint index) : db(db), index(index) {}

        DbTuple2 operator *() const {
            return (*this->db)[this->index];
        }

        Iter<DbTuple2>& operator ++() {
            this->index++;
            return *this;
        }

        template <class DbTuple3> requires std::is_same_v<std::remove_cv_t<DbTuple3>, DbTuple>
        friend bool operator ==(const Iter<DbTuple3>& iter1, const Iter<DbTuple3>& iter2) {
            return iter1.db == iter2.db && iter1.index == iter2.index;
        }

        template <class DbTuple3> requires std::is_same_v<std::remove_cv_t<DbTuple3>, DbTuple>
        friend bool operator !=(const Iter<DbTuple3>& iter1, const Iter<DbTuple3>& iter2) {
            return !(iter1 == iter2);
        }
    };

    // non-const iterators
    Iter<DbTuple> begin() {
        return Iter<DbTuple>(this, 0);
    }

    Iter<DbTuple> end() {
        return Iter<DbTuple>(this, this->_size);
    }

    // const iterators
    Iter<const DbTuple> begin() const {
        return Iter<const DbTuple>(this, 0);
    }

    Iter<const DbTuple> end() const {
        return Iter<const DbTuple>(this, this->_size);
    }
};
