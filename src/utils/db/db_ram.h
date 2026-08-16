#pragma once

#include <concepts>
#include <functional>
#include <initializer_list>
#include <vector>

#include "utils/db/db_interface.h"
#include "utils/tuple.h"
#include "utils/types.h"


template <IsDbTuple DbTuple>
class DbRam : public IDb<DbTuple> {
private:
    using InnerType = std::vector<DbTuple>;

public:
    //--------------------------------------------------------------------------
    // constructors/destructors

    DbRam() = default;

    /**
     * copy `db` from `startIndex` (inclusive) to `endIndex` (exclusive).
     */
    DbRam(const DbRam& db, bigint startIndex, bigint endIndex);

    /**
     * initialize a `DbRam` with the raw values in the brace-enclosed initializer list `initList`.
     */
    DbRam(std::initializer_list<DbTuple> initList);


    //--------------------------------------------------------------------------
    // copy/move

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

    void shuffle() override;
    void sort(
        const std::function<bool(const DbTuple& dbTuple1, const DbTuple& dbTuple2)>& compare
    ) override;

private:
    InnerType vec;
};
