#pragma once

#include "sse.h"

template <IDbDocDeriv DbDoc = IdOp, class DbKw = Kw, template<class A, class B> class Undrly = PiBasResHiding>
class Sda : public IDsse<DbDoc, DbKw> {
    private:
        std::vector<Undrly<DbDoc, DbKw>&> undrlys;

    public:
        // API functions
        void update(const DbEntry<DbDoc, DbKw>& newEntry) override;
}
