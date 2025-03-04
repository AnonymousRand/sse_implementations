#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <IMainDbDoc_ DbDoc = IdOp, class DbKw = Kw, template<class ...> class Underly = PiBas>
        requires ISse_<Underly, DbDoc, DbKw>
class LogSrci : public ISdaUnderly<DbDoc, DbKw> {
    private:
        Underly<SrciDb1Doc<DbKw>, DbKw>& underly1;
        Underly<DbDoc, Id>& underly2;
        TdagNode<DbKw>* tdag1;
        TdagNode<Id>* tdag2;
        Db<DbDoc, DbKw> db; // store since neither underlying instance contains the original DB
        bool _isEmpty = false;

    public:
        LogSrci(Underly<SrciDb1Doc<DbKw>, DbKw>& underly1, Underly<DbDoc, Id>& underly2);

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;

        Db<DbDoc, DbKw> getDb() const override;
        bool isEmpty() const override;
};
