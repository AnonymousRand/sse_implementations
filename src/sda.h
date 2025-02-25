#pragma once

#include "sse.h"

template <IDbDocDeriv DbDoc = IdOp, class DbKw = Kw, template<class A, class B> class Underly = PiBasResHiding>
class Sda : public IDsse<DbDoc, DbKw> {
    private:
        std::vector<Underly<DbDoc, DbKw>&> underlys;

    public:
        // API functions
        void update(const DbEntry<DbDoc, DbKw>& newEntry) override;
}
