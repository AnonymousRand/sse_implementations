#pragma once

#include <concepts>
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <string>
#include <vector>

#include "utils/types/basic_types.h"
#include "utils/types/db/i_db.h"
#include "utils/types/enc_ind/enc_ind_base.h"
#include "utils/types/i_disk_storage.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


// IMPORTANT: inheritance order must have `IDiskStorage` before `IDb` as its move/copy assignment
// operators must be called first, as they call `clear()`!
// (and `= default` calls their parent versions in order of inheritance)
template <IsDbTuple DbTuple>
class DbDisk : public IDiskStorage, public IDb<DbTuple> {
private:
    using IDb<DbTuple>::DbKw;

public:
    //--------------------------------------------------------------------------
    // constructors/destructors

    DbDisk();

    /**
     * copy `db` from `startIndex` (inclusive) to `endIndex` (exclusive).
     */
    DbDisk(const DbDisk& other, bigint startIndex, bigint endIndex);

    /**
     * initialize a `DbDisk` with the raw values in the brace-enclosed initializer list `initList`.
     */
    DbDisk(std::initializer_list<DbTuple> initList);

    //--------------------------------------------------------------------------
    // the big five

    // destructor
    ~DbDisk() = default;

    // copy constructor
    DbDisk(const DbDisk& other);

    // copy assignment operator
    DbDisk& operator =(const DbDisk& other) = default;

    // move constructor
    DbDisk(DbDisk&& other) noexcept = default;

    // move assignment operator
    DbDisk& operator =(DbDisk&& other) noexcept = default;

    //--------------------------------------------------------------------------
    // `IDb`

    void clear() override;
    void append(const DbTuple& dbTuple) override;

    DbTuple operator [](bigint index) const override;

    // (there's nothing we can do in this case with disk storage)
    void reserve(bigint size) override {}

    // these are instantiations of `applyAlgoViaIndices()` (see below) using specific common
    // `algoOnIndices` functions
    // additionally, these methods replace `*this` with the output `DbDisk`
    void shuffle() override;
    void sort(
        const std::function<bool(const DbTuple& dbTuple1, const DbTuple& dbTuple2)>& compare
    ) override;

private:
    constexpr std::string FILE_DIR() const override { return "out/client"; }
    constexpr std::string FILENAME_PREFIX() const override { return "db_"; }

    inline static const int TUPLE_LEN = EncIndBase::DATA_LEN;

    //--------------------------------------------------------------------------
    // helpers

    /**
     * wrapper for algorithms (e.g. from `<algorithms>`) to make them work when a `DbDisk` isn't
     * actually storing any of its entries as actual `DbTuple` elements anywhere.
     *
     * instead, perform the algorithm on a vector of indices (which should be significantly smaller
     * than the `DbDisk` itself) and then using that to build and return a new output `DbDisk`.
     */
    DbDisk<DbTuple> applyAlgoViaIndices(
        const std::function<void(std::vector<bigint>& dbIndices)>& algoOnIndices
    ) const;
};
