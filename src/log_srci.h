#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <IMainDbDocDeriv DbDoc = IdOp, class DbKw = Kw, template<class ...> class Undrly = PiBas>
        requires ISseDeriv<Undrly, DbDoc, DbKw>
class LogSrci : public ISse<DbDoc, DbKw> {
    protected:
        Undrly<SrciDb1Doc<DbKw>, DbKw>& undrly1;
        Undrly<DbDoc, Id>& undrly2;
        TdagNode<DbKw>* tdag1;
        TdagNode<Id>* tdag2;

    public:
        LogSrci(Undrly<SrciDb1Doc<DbKw>, DbKw>& undrly1, Undrly<DbDoc, Id>& undrly2);

        // API functions
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};
