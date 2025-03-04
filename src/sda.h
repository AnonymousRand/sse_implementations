#pragma once

#include "pi_bas.h"
#include "sse.h"

template <IMainDbDoc_ DbDoc, class DbKw, template<class ...> class Underly> requires ISdaUnderly_<Underly, DbDoc, DbKw>
class SdaBase : public IDsse<DbDoc, DbKw> {
    protected:
        std::vector<Underly<DbDoc, DbKw>> underlys;
        int firstEmptyInd = 0;

        std::vector<DbDoc> searchWithoutHandlingDels(const Range<DbKw>& query) const;

    public:
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        void update(const DbEntry<DbDoc, DbKw>& newEntry) override;
};

template <IMainDbDoc_ DbDoc = IdOp, class DbKw = Kw, template<class ...> class Underly = PiBasResHiding>
        requires ISdaUnderly_<Underly, DbDoc, DbKw>
class Sda : public SdaBase<DbDoc, DbKw, Underly> {
    public:
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};

template <class DbKw, template<class ...> class Underly> requires ISdaUnderly_<Underly, IdOp, DbKw>
class Sda<IdOp, DbKw, Underly> : public SdaBase<IdOp, DbKw, Underly> {
    public:
        std::vector<IdOp> search(const Range<DbKw>& query) const;
};
