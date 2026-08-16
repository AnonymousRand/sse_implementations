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

    // these are still manually written even though all of them are ` = default` because the
    // parent `IDb`'s manually declared move assignment operator prevents compiler from
    // automatically generating all of these
    //
    // the default behavior should call the parent(s)' version(s) before doing a per-member
    // copy/move of the child's members

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
