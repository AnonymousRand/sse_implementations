#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <iterator>
#include <string>
#include <unordered_map>

#include "utils/disk_storage.h"
#include "utils/enc_ind.h"
#include "utils/range.h"
#include "utils/tuple.h"


//==============================================================================
// `Db`
//==============================================================================


// this is essentially a vector but stored on disk, hence we match `std::vector`'s method names
template <IsDbTuple DbTuple = Tuple<>>
class Db : public IDiskStorage {
public:
    inline static const int TUPLE_LEN = EncInd::DATA_LEN;

    Db();
    Db(const Db& db);

    /**
     * copy `db` from `startIndex` (inclusive) to `endIndex` (exclusive).
     *
     * (this does not match an `std::vector` constructor since the iterator range one would require
     * decoding and re-encoding every tuple without change, and that has some expensive regex.)
     */
    Db(const Db& db, int64_t startIndex, int64_t endIndex);

    /**
     * initialize with the raw values contained in the brace-enclosed initializer list `initList`.
     */
    Db(std::initializer_list<DbTuple> initList);

    void clear() override;
    void push_back(const DbTuple& dbTuple);

    int64_t size() const {
        return this->_size;
    }

    // read-only accessing using `[]`
    DbTuple operator [](int64_t index) const;

private:
    constexpr std::string FILE_DIR() const override { return "out/client"; }
    constexpr std::string FILENAME_PREFIX() const override { return "db_"; }

    int64_t _size = 0;

    // helpers that don't do encoding/decoding (e.g. also good for faster copy constructors)
    std::string readRaw(int64_t index) const;
    void writeRaw(const std::string& dbTupleStr);

public:
    //--------------------------------------------------------------------------
    // iterator

    // this allows us to iterate through a `Db` using range-based `for` loop or iterators
    // (note this is not a `LegacyRandomAccessIterator`, meaning things like `std::sort()`
    // which need to modify it in place will not work. this is because that requires dereferencing
    // the iterator to return a reference to a `DbTuple` object, but `DbTuple` objects are only
    // constructed on the fly when reading from a `Db`, so this is basically impossible.)

    // the template allows `DbTuple2` to be either `DbTuple` or `const DbTuple` so we can have both
    // non-const (e.g. for `std::sort()`) and const iterators (e.g. for iterating a `const Db<>&`)
    template <class DbTuple2> requires std::is_same_v<std::remove_cv_t<DbTuple2>, DbTuple>
    struct Iter {
        const Db<DbTuple>* db = nullptr;
        int64_t index;

        Iter(const Db<DbTuple>* db, int64_t index) : db(db), index(index) {}

        DbTuple2 operator *() const {
            return (*this->db)[this->index];
        }

        Iter<DbTuple2>& operator ++() {
            this->index++;
            return *this;
        }

        template <class DbTuple3> requires std::is_same_v<std::remove_cv_t<DbTuple3>, DbTuple>
        friend bool operator !=(const Iter<DbTuple3>& iter1, const Iter<DbTuple3>& iter2) {
            return !(iter1.db == iter2.db && iter1.index == iter2.index);
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


//==============================================================================
// `Ind`
//==============================================================================


template <IsDbTuple DbTuple>
using Ind = std::unordered_map<Range<typename DbTuple::DbKwType>, Db<DbTuple>>;
