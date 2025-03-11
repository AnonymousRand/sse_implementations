#pragma once

#include "pi_bas.h"
#include "sse.h"

// don't use template template parameter for `Underly` because they may have other deeper templates (like for Log-SRC)
// and it can get very complicated so probably best to just explicitly specify everything here
template <class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISdaUnderly_<Underly, DbDoc, DbKw>
class SdaBase : public IDsse<DbDoc, DbKw> {
    protected:
        std::vector<Underly> underlys;
        int firstEmptyInd = 0;

        std::vector<DbDoc> searchWithoutHandlingDels(const Range<DbKw>& query) const;

    public:
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        void update(const DbEntry<DbDoc, DbKw>& newEntry) override;
};

template <class Underly, IMainDbDoc_ DbDoc = IdOp, class DbKw = Kw> requires ISdaUnderly_<Underly, DbDoc, DbKw>
class Sda : public SdaBase<Underly, DbDoc, DbKw> {
    public:
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};

template <class Underly, class DbKw> requires ISdaUnderly_<Underly, IdOp, DbKw>
class Sda<Underly, IdOp, DbKw> : public SdaBase<Underly, IdOp, DbKw> {
    public:
        std::vector<IdOp> search(const Range<DbKw>& query) const;
};
