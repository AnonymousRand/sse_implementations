#pragma once

#include <concepts>
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <string>
#include <vector>

#include "utils/db/db_interface.h"
#include "utils/disk_storage.h"
#include "utils/enc_ind.h"
#include "utils/range.h"
#include "utils/tuple.h"
#include "utils/types.h"


template <IsDbTuple DbTuple>
class DbDisk : public IDb<DbTuple>, public IDiskStorage {
public:
    //--------------------------------------------------------------------------
    // constructors/destructors

    DbDisk();

    /**
     * copy `db` from `startIndex` (inclusive) to `endIndex` (exclusive).
     *
     * (this does not match an `std::vector` constructor since the iterator range one would require
     * decoding and re-encoding every tuple without change, and that has some expensive regex.)
     */
    DbDisk(const DbDisk& db, bigint startIndex, bigint endIndex);

    /**
     * initialize a `DbDisk` with the raw values in the brace-enclosed initializer list `initList`.
     */
    DbDisk(std::initializer_list<DbTuple> initList);

    //--------------------------------------------------------------------------
    // copy/move

    // destructor
    ~DbDisk() = default;

    // copy constructor
    DbDisk(const DbDisk& other);

    // copy assignment operator
    DbDisk& operator =(const DbDisk& other) = default;

    // move constructor
    DbDisk(DbDisk&& other) noexcept = default;

    // move assignment operator
    // (not `= default` here as we have multiple inheritance, and the order in which parent move
    // assignment operators are called is very important here)
    // TODO: test performance vs. without this!
    DbDisk& operator =(DbDisk&& other) noexcept;

    //--------------------------------------------------------------------------
    // `IDb`

    void clear() override;
    void push_back(const DbTuple& dbTuple) override;

    DbTuple operator [](bigint index) const override;

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

    inline static const int TUPLE_LEN = EncInd::DATA_LEN;

    //--------------------------------------------------------------------------
    // helpers

    // helpers that don't do encoding/decoding (e.g. also good for faster copy constructors)
    void readRaw(bigint index, char* ret) const;
    void appendRaw(const char* dbTupleCstr);

    /**
     * wrapper for algorithms (e.g. from `<algorithms>`) to make them work when a `DbDisk` isn't
     * actually storing any of its entries as actual `DbTuple` elements anywhere.
     *
     * instead, perform the algorithm on a vector of indices (which should be significantly
     * smaller than the `DbDisk` itself) and then using that to build and return a new output `DbDisk`.
     */
    DbDisk<DbTuple> applyAlgoViaIndices(
        const std::function<void(std::vector<bigint>& dbIndices)>& algoOnIndices
    ) const;
};
