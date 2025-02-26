#pragma once

#include <functional>

#include "pi_bas.h"
#include "sse.h"

template <IMainDbDocDeriv DbDoc, class DbKw, template<class ...> class Undrly> requires ISseDeriv<Undrly, DbDoc, DbKw>
class SdaBase : public IDsse<DbDoc, DbKw> {
    protected:
        std::vector<std::reference_wrapper<Undrly<DbDoc, DbKw>>> undrlys;
        int firstEmptyInd;

        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const;

    public:
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        void update(const DbEntry<DbDoc, DbKw>& newEntry) override;
};

template <IMainDbDocDeriv DbDoc = IdOp, class DbKw = Kw, template<class ...> class Undrly = PiBasResHiding>
        requires ISseDeriv<Undrly, DbDoc, DbKw>
class Sda : public SdaBase<DbDoc, DbKw, Undrly> {
    public:
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};

template <class DbKw, template<class ...> class Undrly> requires ISseDeriv<Undrly, IdOp, DbKw>
class Sda<IdOp, DbKw, Undrly> : public SdaBase<IdOp, DbKw, Undrly> {
    public:
        std::vector<IdOp> search(const Range<DbKw>& query) const;
};
