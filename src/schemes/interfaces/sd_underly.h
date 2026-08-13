#pragma once

#include <concepts>
#include <cstdint>

#include "schemes/interfaces/sse.h"

#include "utils/db.h"
#include "utils/types.h"


// underlying SSE schemes for SD-type DSSE schemes (from NDSS'20)
template <IsDbTuple DbTuple = Tuple<>>
class ISdUnderly : public virtual ISse<DbTuple> {
protected:
    using DbKw = typename ISse<DbTuple>::DbKw;

public:
    using ISse<DbTuple>::ISse;

    //----------------------------------------------------------------------
    // methods to implement

    /**
     * append the `db` most recently passed to `setup()` (without any replications/
     * padding/processing) to `ret`.
     */
    virtual void getDb(Db<DbTuple>& ret) const = 0;

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
