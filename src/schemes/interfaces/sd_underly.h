#pragma once

#include <concepts>
#include <cstdint>

#include "schemes/interfaces/sse.h"

#include "utils/doc.h"
#include "utils/sse_utils.h"


// underlying SSE schemes for SD-type DSSE schemes (from NDSS'20)
template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class ISdUnderly : public virtual ISse<DbDoc, DbKw> {
public:
    using ISse<DbDoc, DbKw>::ISse;

    //----------------------------------------------------------------------
    // methods to implement

    /**
     * append the `db` most recently passed to `setup()` (without any replications/padding/processing) to `ret`.
     */
    virtual void getDb(Db<DbDoc, DbKw>& ret) const = 0;

    //----------------------------------------------------------------------
    // shared code

    // handle clearing of `size` member variable belonging to this interface
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
