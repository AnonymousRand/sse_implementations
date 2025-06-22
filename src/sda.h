#pragma once

#include "pi_bas.h"
#include "sse.h"

// don't use template template parameter for `Underly` because they may have other deeper templates (like for Log-SRC)
// and it can get very complicated so probably best to just explicitly specify everything here
// TODO can we do template<class EncInd, class ...> Underly to get rid of need for second EncInd tempalte arg?
template <class Underly, IEncInd_ EncInd, IMainDbDoc_ DbDoc, class DbKw>
    requires ISdaUnderly_<Underly, EncInd, DbDoc, DbKw>
class SdaBase : public IDsse<EncInd, DbDoc, DbKw> {
    protected:
        std::vector<Underly*> underlys;
        int firstEmptyInd = 0;

        std::vector<DbDoc> searchWithoutHandlingDels(const Range<DbKw>& query) const;

    public:
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        void update(const DbEntry<DbDoc, DbKw>& newEntry) override;
};

template <class Underly, IEncInd_ EncInd, IMainDbDoc_ DbDoc = IdOp, class DbKw = Kw>
        requires ISdaUnderly_<Underly, EncInd, DbDoc, DbKw>
class Sda : public SdaBase<Underly, EncInd, DbDoc, DbKw> {
    public:
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};

template <class Underly, IEncInd_ EncInd, class DbKw> requires ISdaUnderly_<Underly, EncInd, IdOp, DbKw>
class Sda<Underly, EncInd, IdOp, DbKw> : public SdaBase<Underly, EncInd, IdOp, DbKw> {
    public:
        std::vector<IdOp> search(const Range<DbKw>& query) const;
};
