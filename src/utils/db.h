#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <string>
#include <unordered_map>

#include "utils/disk_storage.h"
#include "utils/enc_ind.h"
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

    void clear() override;
    void push_back(const DbTuple& dbTuple);

    int64_t size() const {
        return this->_size;
    }

    // read-only accessing using `[]`
    const DbTuple& operator [](int64_t index) const;

private:
    constexpr std::string FILENAME_DIR() const override { return "out/client"; }
    constexpr std::string FILENAME_PREFIX() const override { return "db_"; }

    int64_t _size = 0;

public:
    //--------------------------------------------------------------------------
    // iterator

    // this allows iterating using range-based `for` loop or iterators, as well as `std::sort()`
    // (specifically, `std::sort()` requires this to be a `LegacyRandomAccessIterator`, which
    // requires us to implement all the methods, operator overloads, and aliases below)
    struct Iter {
        const Db<DbTuple>* db = nullptr;
        int64_t index;

        using iterator_category = std::random_access_iterator_tag;
        using value_type        = DbTuple;
        using pointer           = value_type*;
        using reference         = value_type&;
        using difference_type   = int64_t;

        Iter(const Db<DbTuple>* db, int64_t index) : db(db), index(index) {}

        // dereferencing
        reference operator *() const {
            return (*this->db)[this->index];
        }
        pointer operator ->() const {
            return &((*this->db)[this->index]);
        }
        reference operator [](difference_type n) const {
            return (*this->db)[this->index + n];
        }

        // incrementing/decrementing
        Iter& operator ++() {
            this->index++;
            return *this;
        }
        Iter operator ++(int) {
            Iter tmpIter = *this;
            ++(*this);
            return tmpIter;
        }
        Iter& operator --() {
            this->index--;
            return *this;
        }
        Iter operator --(int) {
            Iter tmpIter = *this;
            --(*this);
            return tmpIter;
        }

        // compound assignment
        Iter& operator +=(difference_type n) {
            this->index += n;
            return *this;
        }
        Iter& operator -=(difference_type n) {
            this->index -= n;
            return *this;
        }

        // arithmetic
        friend Iter operator +(Iter lhs, difference_type n) {
            lhs += n;
            return lhs;
        }
        friend Iter operator +(difference_type n, Iter rhs) {
            return rhs + n;
        }
        friend Iter operator -(Iter lhs, difference_type n) {
            lhs -= n;
            return lhs;
        }
        friend difference_type operator -(const Iter& iter1, const Iter& iter2) {
            return iter1.index - iter2.index;
        }

        // equality
        friend bool operator ==(const Iter& iter1, const Iter& iter2) {
            return iter1.db == iter2.db && iter1.index == iter2.index;
        }
        friend bool operator !=(const Iter& iter1, const Iter& iter2) {
            return !(iter1 == iter2);
        }

        // inequalities
        friend bool operator <(const Iter& iter1, const Iter& iter2) {
            return iter1.db == iter2.db && iter1.index < iter2.index;
        }
        friend bool operator <=(const Iter& iter1, const Iter& iter2) {
            return iter1 < iter2 || iter1 == iter2;
        }
        friend bool operator >(const Iter& iter1, const Iter& iter2) {
            return !(iter1 <= iter2);
        }
        friend bool operator >=(const Iter& iter1, const Iter& iter2) {
            return !(iter1 < iter2);
        }
    };

    Iter begin() {
        return Iter(this, 0);
    }

    Iter end() {
        return Iter(this, this->size);
    }
};


//==============================================================================
// `Ind`
//==============================================================================


template <IsDbTuple DbTuple>
using Ind = std::unordered_map<DbTuple, Db<DbTuple>>;
