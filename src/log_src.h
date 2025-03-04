#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <IMainDbDoc_ DbDoc = IdOp, class DbKw = Kw, template<class ...> class Underly = PiBas>
        requires ISse_<Underly, DbDoc, DbKw>
class LogSrc : public IUnderly<DbDoc, DbKw> {
    private:
        Underly<DbDoc, DbKw>& underly;
        TdagNode<DbKw>* tdag;

    public:
        LogSrc(Underly<DbDoc, DbKw>& underly);

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;

        Db<DbDoc, DbKw> getDb() const override;
        bool isEmpty() const override;
};
