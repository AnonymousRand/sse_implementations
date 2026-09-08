#pragma once

#include <concepts>

#include "schemes/interfaces/sse.h"

#include "types/basic_types.h"
#include "types/db/db.h"
#include "types/tuple.h"


// underlying SSE schemes for SD-type DSSE schemes (from NDSS'20)
template <IsDbTuple DbTuple = Tuple<>>
class ISdUnderly : public virtual ISse<DbTuple> {
protected:
    using DbKw = typename ISse<DbTuple>::DbKw;

public:
    //--------------------------------------------------------------------------
    // interface

    /**
     * append the DB most recently passed to `setup()` (WITHOUT any replications/
     * padding/processing!) to `ret`.
     */
    virtual void getDb(Db<DbTuple>& ret) const = 0;

    //--------------------------------------------------------------------------
    // shared code

    // handle clearing of `this->size`
    void clear() override { this->size = 0; }

    bigint getSize() const { return this->size; }

protected:
    /**
     * the size of the DB most recently passed to `setup()` (WITHOUT any replications/
     * padding/processing!).
     */
    bigint size;
};


template <class T>
concept IsSdUnderly = requires(T t) {
    []<class ... Args>(ISdUnderly<Args ...>&){}(t);
};
