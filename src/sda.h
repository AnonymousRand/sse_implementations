#pragma once

#include "sse.h"

template <IMainDbDocDeriv DbDoc = IdOp, class DbKw = Kw, template<class ...> class Undrly = PiBasResHiding>
        requires ISseDeriv<Undrly, DbDoc, DbKw>
class Sda : public IDsse<DbDoc, DbKw> {
    private:
        std::vector<Undrly<DbDoc, DbKw>&> undrlys;
        int firstEmptyInd;

    public:
        // API functions
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
        void update(const DbEntry<DbDoc, DbKw>& newEntry) override;
}
