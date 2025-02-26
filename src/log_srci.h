#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <IMainDbDoc_ DbDoc = IdOp, class DbKw = Kw, template<class ...> class Underly = PiBas>
        requires IUnderly_<Underly, DbDoc, DbKw>
class LogSrci : public ISse<DbDoc, DbKw> {
    private:
        Underly<SrciDb1Doc<DbKw>, DbKw>& underly1;
        Underly<DbDoc, Id>& underly2;
        TdagNode<DbKw>* tdag1;
        TdagNode<Id>* tdag2;

    public:
        LogSrci(Underly<SrciDb1Doc<DbKw>, DbKw>& underly1, Underly<DbDoc, Id>& underly2);

        // API functions
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};
