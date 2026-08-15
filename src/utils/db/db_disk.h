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

    /**
     * copy constructor (that should avoid encoding and de-encoding each tuple as it is moved).
     */
    DbDisk(const DbDisk& other);

    /**
     * move assignment operator that allows cheap moving instead of expensive copying of
     * `DbDisk` rvalues when they are assigned to an existing variable, e.g. `*this = ...`.
     */
    // (currently, this is `= default` to force the compiler to generate a default one, which is
    // needed to make sure parent class move assignment operators are also called. normally the
    // compiler autogenerates such default constructors, but i've manually implemented other special
    // constructors like the copy constructor in this class, which prevents this autogeneration.)
    // TODO: test performance vs. without this!
    DbDisk& operator =(DbDisk&& other) noexcept = default;

    //--------------------------------------------------------------------------
    // `IDb`

    void clear() override;
    void push_back(const DbTuple& dbTuple) override;

    DbTuple operator [](bigint index) const override;

    // these are instantiations of `applyAlgoViaIndices()` (see below) using specific common
    // `algoOnIndices` functions
    // additionally, these methods replace `*this` with the output `DbDisk`
    void shuffle() override;
    void sort(const std::function<bool(bigint index1, bigint index2)>& compare) override;

private:
    constexpr std::string FILE_DIR() const override { return "out/client"; }
    constexpr std::string FILENAME_PREFIX() const override { return "db_"; }

    inline static const int TUPLE_LEN = EncInd::DATA_LEN;

    // (`mutable` allows this to be modified in `const` contexts still)
    mutable bool isFlushed = true;

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
