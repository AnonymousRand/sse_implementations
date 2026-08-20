#pragma once

#include <concepts>
#include <memory>
#include <vector>

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


// forward declare instead of include to avoid a circular include with `benchmark.h`
struct Benchmark;


template <IsDbTuple DbTuple = Tuple<>>
class ISse {
protected:
    using DbKw = typename DbTuple::DbKwType;

public:
    std::shared_ptr<Benchmark> benchmark;

    //--------------------------------------------------------------------------
    // methods to implement

    ISse(std::shared_ptr<Benchmark> benchmark) : benchmark(benchmark) {}

    virtual void setup(int secParam, const Db<DbTuple>& db) = 0;
    
    /**
     * params:
     *     - `shouldCleanUpResults`: whether to filter out deleted docs or not
     *     - `isNaive`: whether to search each point in `query` individually,
     *       or the entire range in one go (i.e. `query` itself must be in the db),
     *       e.g. as the underlying scheme for a range scheme like Log-SRC.
     */
    virtual std::vector<DbTuple> search(
        const Range<DbKw>& query, bool shouldCleanUpResults = true, bool isNaive = true
    ) const = 0;

    /**
     * free memory and clear the db/index, without fully destroying this object as the
     * destructor does (so we can still call `setup()` again with the same object,
     * perhaps with a different db).
     * 
     * notes:
     *     - should be idempotent and safe to call without `setup()` first as well.
     */
    virtual void clear() = 0;

protected:
    int secParam;
};


template <class T>
concept IsSse = requires(T t) {
    []<class ... Args>(ISse<Args ...>&){}(t);
};
