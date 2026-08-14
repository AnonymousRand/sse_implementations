#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>

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

    //--------------------------------------------------------------------------
    // constructors/destructors

    Db();

    /**
     * copy constructor (that should avoid encoding and de-encoding each tuple as it is moved).
     */
    Db(const Db& other);

    /**
     * copy `db` from `startIndex` (inclusive) to `endIndex` (exclusive).
     *
     * (this does not match an `std::vector` constructor since the iterator range one would require
     * decoding and re-encoding every tuple without change, and that has some expensive regex.)
     */
    Db(const Db& db, int64_t startIndex, int64_t endIndex);

    /**
     * initialize a `Db` with the raw values in the brace-enclosed initializer list `initList`.
     */
    Db(std::initializer_list<DbTuple> initList);

    /**
     * move assignment operator that allows cheap moving instead of expensive copying of
     * `Db` rvalues when they are assigned to an existing variable, e.g. `*this = ...`.
     */
    // (currently, this is `= default` to force the compiler to generate a default one, which is
    // needed to make sure parent class move assignment operators are also called. normally the
    // compiler autogenerates such default constructors, but i've manually implemented other special
    // constructors like the copy constructor in this class, which prevents this autogeneration.)
    // TODO: test performance vs. without this!
    Db& operator =(Db&& other) noexcept = default;

    //--------------------------------------------------------------------------
    // interface

    void clear() override;
    void push_back(const DbTuple& dbTuple);

    int64_t size() const {
        return this->_size;
    }

    bool empty() const;

    // read-only accessing using `[]`
    DbTuple operator [](int64_t index) const;

    /**
     * wrapper for algorithms (e.g. from `<algorithms>`) to make them work when a `Db` isn't
     * actually storing any of its entries as actual `DbTuple` elements anywhere.
     *
     * instead, perform the algorithm on a vector of indices (which should be significantly
     * smaller than the `Db` itself) and then using that to build and return a new output `Db`.
     */
    Db<DbTuple> applyAlgoViaIndices(
        const std::function<void(std::vector<int64_t>& dbIndices)>& algoOnIndices
    ) const;

    /**
     * instantiations of `applyAlgoViaIndices()` using specific common `algoOnIndices` functions.
     * additionally, these methods replace `*this` with the output `Db`.
     */
    void shuffle();
    void sort(const std::function<bool(int64_t index1, int64_t index2)>& compare);

private:
    constexpr std::string FILE_DIR() const override { return "out/client"; }
    constexpr std::string FILENAME_PREFIX() const override { return "db_"; }

    int64_t _size = 0;

    // TODO rename "other" section to "helpers" everywhere?
    // TODO add more of these sections (e.g. tdag) for grouping?
    //--------------------------------------------------------------------------
    // helpers

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
