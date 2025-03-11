#pragma once

#include "pi_bas.h"
#include "sse.h"

template <IMainDbDoc_ DbDoc, class DbKw, class Underly> requires ISdaUnderly_<Underly>
class SdaBase : public IDsse<DbDoc, DbKw> {
    protected:
        std::vector<Underly> underlys;
        int firstEmptyInd = 0;

        std::vector<DbDoc> searchWithoutHandlingDels(const Range<DbKw>& query) const;

    public:
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        void update(const DbEntry<DbDoc, DbKw>& newEntry) override;
};

template <IMainDbDoc_ DbDoc = IdOp, class DbKw = Kw, class Underly = PiBasResHiding<DbDoc, DbKw>>
        requires ISdaUnderly_<Underly>
class Sda : public SdaBase<DbDoc, DbKw, Underly> {
    public:
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};

template <class DbKw, class Underly> requires ISdaUnderly_<Underly>
class Sda<IdOp, DbKw, Underly> : public SdaBase<IdOp, DbKw, Underly> {
    public:
        std::vector<IdOp> search(const Range<DbKw>& query) const;
};
