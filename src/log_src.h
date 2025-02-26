#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <IMainDbDocDeriv DbDoc = IdOp, class DbKw = Kw, template<class ...> class Underly = PiBas>
        requires IUnderlyDeriv<Underly, DbDoc, DbKw>
class LogSrc : public ISse<DbDoc, DbKw> {
    private:
        Underly<DbDoc, DbKw>& underly;
        TdagNode<DbKw>* tdag;

    public:
        LogSrc(Underly<DbDoc, DbKw>& underly);

        // API functions
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};
