#pragma once

#include <concepts>

#include "schemes/interfaces/i_sse.h"

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/enc_ind/enc_ind_base.h"
#include "utils/types/tuple.h"


// underlying SSE schemes for SD-type DSSE schemes (from NDSS'20)
template <IsDbTuple DbTuple = Tuple<>>
class ISdUnderly : public virtual ISse<DbTuple> {
protected:
    using DbDoc = typename ISse<DbTuple>::DbDoc;
    using DbKw = typename ISse<DbTuple>::DbKw;

public:
    //--------------------------------------------------------------------------
    // `ISse`

    // handle clearing of `this->size`
    void clear() override {
        this->size = 0;
    }

    //--------------------------------------------------------------------------
    // interface

    /**
     * append the DB most recently passed to `setup()` (WITHOUT any replications/
     * padding/processing!) to `ret`.
     */
    virtual void getDb(Db<DbTuple>& ret) const = 0;

    bigint getSize() const { return this->size; }
    void setSetupOper(EncIndBase::Oper setupOper) { this->setupOper = setupOper; }

protected:
    /**
     * the size of the DB most recently passed to `setup()` (WITHOUT any replications/
     * padding/processing!).
     */
    bigint size;
    EncIndBase::Oper setupOper = EncIndBase::Oper::SETUP;
};


template <class T>
concept IsSdUnderly = requires(T t) {
    []<class ... Args>(ISdUnderly<Args ...>&){}(t);
};
