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

    void clear() override;
    void push_back(const DbTuple& dbTuple);

    int64_t size() const {
        return this->_size;
    }

    // read-only accessing using `[]`
    DbTuple& operator [](int64_t index) const;

private:
    constexpr std::string FILE_DIR() const override { return "out/client"; }
    constexpr std::string FILENAME_PREFIX() const override { return "db_"; }

    int64_t _size = 0;

public:
    //--------------------------------------------------------------------------
    // iterator

    // this allows iterating using range-based `for` loop or iterators, as well as `std::sort()`
    // (specifically, `std::sort()` requires this to be a `LegacyRandomAccessIterator`, which
    // requires us to implement all the methods, operator overloads, and aliases below)
    // the template allows `DbTuple2` to be either `DbTuple` or `const DbTuple` so we can have both
    // non-const (e.g. for `std::sort()`) and const iterators (e.g. for iterating a `const Db<>&`)
    template <class DbTuple2> requires std::is_same_v<std::remove_cv_t<DbTuple2>, DbTuple>
    struct Iter {
        const Db<DbTuple>* db = nullptr;
        int64_t index;

        using iterator_category = std::random_access_iterator_tag;
        using value_type        = DbTuple;
        using pointer           = DbTuple2*;
        using reference         = DbTuple2&;
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
        Iter<DbTuple2>& operator ++() {
            this->index++;
            return *this;
        }
        Iter<DbTuple2> operator ++(int) {
            Iter<DbTuple2> tmpIter = *this;
            ++(*this);
            return tmpIter;
        }
        Iter<DbTuple2>& operator --() {
            this->index--;
            return *this;
        }
        Iter<DbTuple2> operator --(int) {
            Iter<DbTuple2> tmpIter = *this;
            --(*this);
            return tmpIter;
        }

        // compound assignment
        Iter<DbTuple2>& operator +=(difference_type n) {
            this->index += n;
            return *this;
        }
        Iter<DbTuple2>& operator -=(difference_type n) {
            this->index -= n;
            return *this;
        }

        // arithmetic
        template <class DbTuple3> requires std::is_same_v<std::remove_cv_t<DbTuple3>, DbTuple>
        friend Iter<DbTuple3> operator +(Iter<DbTuple3> lhs, difference_type n) {
            lhs += n;
            return lhs;
        }
        template <class DbTuple3> requires std::is_same_v<std::remove_cv_t<DbTuple3>, DbTuple>
        friend Iter<DbTuple3> operator +(difference_type n, Iter<DbTuple3> rhs) {
            return rhs + n;
        }
        template <class DbTuple3> requires std::is_same_v<std::remove_cv_t<DbTuple3>, DbTuple>
        friend Iter<DbTuple3> operator -(Iter<DbTuple3> lhs, difference_type n) {
            lhs -= n;
            return lhs;
        }
        template <class DbTuple3> requires std::is_same_v<std::remove_cv_t<DbTuple3>, DbTuple>
        friend difference_type operator -(
            const Iter<DbTuple3>& iter1, const Iter<DbTuple3>& iter2
        ) {
            return iter1.index - iter2.index;
        }

        // equality
        template <class DbTuple3> requires std::is_same_v<std::remove_cv_t<DbTuple3>, DbTuple>
        friend bool operator ==(const Iter<DbTuple3>& iter1, const Iter<DbTuple3>& iter2) {
            return iter1.db == iter2.db && iter1.index == iter2.index;
        }
        template <class DbTuple3> requires std::is_same_v<std::remove_cv_t<DbTuple3>, DbTuple>
        friend bool operator !=(const Iter<DbTuple3>& iter1, const Iter<DbTuple3>& iter2) {
            return !(iter1 == iter2);
        }

        // inequalities
        template <class DbTuple3> requires std::is_same_v<std::remove_cv_t<DbTuple3>, DbTuple>
        friend bool operator <(const Iter<DbTuple3>& iter1, const Iter<DbTuple3>& iter2) {
            return iter1.db == iter2.db && iter1.index < iter2.index;
        }
        template <class DbTuple3> requires std::is_same_v<std::remove_cv_t<DbTuple3>, DbTuple>
        friend bool operator <=(const Iter<DbTuple3>& iter1, const Iter<DbTuple3>& iter2) {
            return iter1 < iter2 || iter1 == iter2;
        }
        template <class DbTuple3> requires std::is_same_v<std::remove_cv_t<DbTuple3>, DbTuple>
        friend bool operator >(const Iter<DbTuple3>& iter1, const Iter<DbTuple3>& iter2) {
            return !(iter1 <= iter2);
        }
        template <class DbTuple3> requires std::is_same_v<std::remove_cv_t<DbTuple3>, DbTuple>
        friend bool operator >=(const Iter<DbTuple3>& iter1, const Iter<DbTuple3>& iter2) {
            return !(iter1 < iter2);
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
