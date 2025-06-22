#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <template <class ...> class Underly, IEncInd_ EncInd, IMainDbDoc_ DbDoc = IdOp, class DbKw = Kw>
        requires ISse_<Underly, EncInd, DbDoc, DbKw>
class LogSrc : public ISdaUnderly<EncInd, DbDoc, DbKw> {
    private:
        Underly<EncInd, DbDoc, DbKw> underly;
        TdagNode<DbKw>* tdag;
        Db<DbDoc, DbKw> db; // store instead of using underlying instance's `db` since that one has replications

        LogSrc(const Underly<EncInd, DbDoc, DbKw>& underly);

    public:
        LogSrc();
        ~LogSrc();

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;

        std::vector<DbDoc> searchWithoutHandlingDels(const Range<DbKw>& query) const override;
        Db<DbDoc, DbKw> getDb() const override;
        bool isEmpty() const override;
};
