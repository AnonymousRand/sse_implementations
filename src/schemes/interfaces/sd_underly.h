#pragma once

#include <concepts>
#include <cstdint>

#include "schemes/interfaces/sse.h"

#include "utils/db.h"
#include "utils/sse_utils.h"


// underlying SSE schemes for SD-type DSSE schemes (from NDSS'20)
template <class DbTuple = Tuple<>, class DbKw = Kw> requires IsValidDbParams<DbTuple, DbKw>
class ISdUnderly : public virtual ISse<DbTuple, DbKw> {
public:
    using ISse<DbTuple, DbKw>::ISse;

    //----------------------------------------------------------------------
    // methods to implement

    /**
     * append the `db` most recently passed to `setup()` (without any replications/
     * padding/processing) to `ret`.
     */
    virtual void getDb(Db<DbTuple, DbKw>& ret) const = 0;

    //----------------------------------------------------------------------
    // shared code

    // handle clearing of `this->size`
    void clear() override {
        this->size = 0;
    }

    int64_t getSize() const {
        return this->size;
    }

protected:
    int64_t size;
};


template <class T>
concept IsSdUnderly = requires(T t) {
    []<class ... Args>(ISdUnderly<Args ...>&){}(t);
};
