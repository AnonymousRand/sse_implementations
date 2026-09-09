#pragma once

#include <concepts>
#include <vector>

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/doc.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


template <IsDbTuple DbTuple = Tuple<>>
class ISse {
protected:
    using DbDoc = typename DbTuple::DbDocType;
    using DbKw = typename DbTuple::DbKwType;

public:
    //--------------------------------------------------------------------------
    // rule of five

    // delete all these to prevent copying and moving! as they cause double frees
    // and all that yummy stuff with raw pointer members
    // IMPORTANT: this means SSE scheme classes can only be instantiated as pointers!

    // bring back default constructor
    ISse() = default;

    // copy constructor
    ISse(const ISse& other) = delete;

    // copy assignment operator
    ISse& operator =(const ISse& other) = delete;

    // move constructor
    ISse(ISse&& other) noexcept = delete;

    // move assignment operator
    ISse& operator =(ISse&& other) noexcept = delete;

    //--------------------------------------------------------------------------
    // interface

    virtual void setup(int secParam, const Db<DbTuple>& db) = 0;
    
    /**
     * params:
     *     - `shouldCleanUpResults`: whether to filter out deleted docs or not
     *     - `isNaive`: whether to search each point in `query` individually,
     *       or the entire range in one go (i.e. `query` itself must be in the db),
     *       e.g. as the underlying scheme for a range scheme like Log-SRC.
     */
    virtual std::vector<DbDoc> search(
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


// black magic to detect if `T` is derived from `ISse` regardless of template params
// (`std::derived_from` only works for non-templated types)
// (Java generics `extends`: look what they need to mimic a fraction of my power)
template <class T>
concept IsSse = requires(T t) {
    []<class ... Args>(ISse<Args ...>&){}(t);
};
