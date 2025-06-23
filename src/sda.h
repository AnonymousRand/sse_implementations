#pragma once

#include "pi_bas.h"
#include "sse.h"

// don't use template template parameter for `Underly` because they may have other deeper underlying schemes
// and it can get very complicated so probably best to just explicitly specify everything here
template <class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISdaUnderly_<Underly>
class SdaBase : public IDsse<DbDoc, DbKw> {
    protected:
        std::vector<Underly*> underlys;
        EncIndType encIndType;
        int firstEmptyInd = 0;

        std::vector<DbDoc> searchWithoutHandlingDels(const Range<DbKw>& query) const;

    public:
        SdaBase(EncIndType encIndType);

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        void update(const DbEntry<DbDoc, DbKw>& newEntry) override;
        void setEncIndType(EncIndType encIndType) override;
};

template <class Underly, IMainDbDoc_ DbDoc = IdOp, class DbKw = Kw> requires ISdaUnderly_<Underly>
class Sda : public SdaBase<Underly, DbDoc, DbKw> {
    public:
        Sda(EncIndType encIndType);

        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};

template <class Underly, class DbKw> requires ISdaUnderly_<Underly>
class Sda<Underly, IdOp, DbKw> : public SdaBase<Underly, IdOp, DbKw> {
    public:
        Sda(EncIndType encIndType);

        std::vector<IdOp> search(const Range<DbKw>& query) const;
};
