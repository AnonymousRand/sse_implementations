#pragma once

#include <concepts>
#include <functional>
#include <initializer_list>
#include <vector>

#include "utils/types/basic_types.h"
#include "utils/types/db/i_db.h"
#include "utils/types/tuple.h"


template <IsDbTuple DbTuple>
class DbRam : public IDb<DbTuple> {
private:
    using IDb<DbTuple>::DbKw;

    using InnerType = std::vector<DbTuple>;

public:
    //--------------------------------------------------------------------------
    // constructors/destructors

    DbRam() = default;

    /**
     * copy `db` from `startIndex` (inclusive) to `endIndex` (exclusive).
     */
    DbRam(const DbRam& other, bigint startIndex, bigint endIndex);

    /**
     * initialize a `DbRam` with the raw values in the brace-enclosed initializer list `initList`.
     */
    DbRam(std::initializer_list<DbTuple> initList);


    //--------------------------------------------------------------------------
    // the big five

    // destructor
    ~DbRam() = default;

    // copy constructor
    DbRam(const DbRam& other) = default;

    // copy assignment operator
    DbRam& operator =(const DbRam& other) = default;

    // move constructor
    DbRam(DbRam&& other) noexcept = default;

    // move assignment operator
    DbRam& operator =(DbRam&& other) noexcept = default;

    //--------------------------------------------------------------------------
    // `IDb`

    void clear() override;
    void push_back(const DbTuple& dbTuple) override;

    DbTuple operator [](bigint index) const override;

    void reserve(bigint size) override;

    void shuffle() override;
    void sort(
        const std::function<bool(const DbTuple& dbTuple1, const DbTuple& dbTuple2)>& compare
    ) override;

private:
    InnerType vec;
};
