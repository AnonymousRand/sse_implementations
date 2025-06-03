#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <template<class ...> class Underly = PiBas, IMainDbDoc_ DbDoc = IdOp, class DbKw = Kw>
        requires ISse_<Underly, DbDoc, DbKw>
class LogSrci : public ISdaUnderly<DbDoc, DbKw> {
    private:
        Underly<SrciDb1Doc<DbKw>, DbKw> underly1;
        Underly<DbDoc, Id> underly2;
        TdagNode<DbKw>* tdag1;
        TdagNode<Id>* tdag2;
        Db<DbDoc, DbKw> db; // store since neither underlying instance contains the original DB
        bool _isEmpty = false;

        LogSrci(const Underly<SrciDb1Doc<DbKw>, DbKw>& underly1, const Underly<DbDoc, Id>& underly2);
        Range<Id> searchBase(const Range<DbKw>& query) const;

    public:
        LogSrci();

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;

        std::vector<DbDoc> searchWithoutHandlingDels(const Range<DbKw>& query) const override;
        Db<DbDoc, DbKw> getDb() const override;
        bool isEmpty() const override;
};
