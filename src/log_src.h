#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <IMainDbDocDeriv DbDoc = IdOp, class DbKw = Kw, template<class ...> class Undrly = PiBas>
        requires ISseDeriv<Undrly, DbDoc, DbKw>
class LogSrc : public ISse<DbDoc, DbKw> {
    private:
        Undrly<DbDoc, DbKw>& undrly;
        TdagNode<DbKw>* tdag;

    public:
        LogSrc(Undrly<DbDoc, DbKw>& undrly);

        // API functions
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;

        Db<DbDoc, DbKw> getDb() const override;
};
