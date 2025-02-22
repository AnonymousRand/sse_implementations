#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <class DbDoc = Doc, class DbKw = Kw, template<class A, class B> class Underly = PiBas>
class LogSrci : public ISse<DbDoc, DbKw> {
    protected:
        Underly<SrciDb1Doc<DbKw>, DbKw>& underlying1;
        Underly<DbDoc, Id>& underlying2;
        TdagNode<DbKw>* tdag1;
        TdagNode<Id>* tdag2;

    public:
        LogSrci(Underly<SrciDb1Doc<DbKw>, DbKw>& underlying1, Underly<DbDoc, Id>& underlying2);

        // API functions
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};
