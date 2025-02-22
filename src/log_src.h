#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <class DbDoc = IdOp, class DbKw = Kw, template<class A, class B> class Underly = PiBas>
class LogSrc : public ISse<DbDoc, DbKw> {
    private:
        Underly<DbDoc, DbKw>& underlying;
        TdagNode<DbKw>* tdag;

    public:
        LogSrc(Underly<DbDoc, DbKw>& underlying);

        // API functions
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};
