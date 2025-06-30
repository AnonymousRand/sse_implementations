#pragma once


#include "sse.h"
#include "util/tdag.h"


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
class LogSrc : public ISse<Doc, Kw>, public ISdaUnderly<Doc, Kw> {
    private:
        Underly<Doc, Kw>* underly = nullptr;
        TdagNode<Kw>* tdag = nullptr;
        Db<Doc, Kw> db; // store instead of using underlying instance's `db` since that one has replications

        LogSrc(Underly<Doc, Kw>* underly, EncIndType encIndType);

    public:
        LogSrc();
        LogSrc(EncIndType encIndType);
        ~LogSrc();

        void setup(int secParam, const Db<Doc, Kw>& db) override;
        std::vector<Doc> search(
            const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
        ) const override;

        void clear() override;
        Db<Doc, Kw> getDb() const override;
        bool isEmpty() const override;
        void setEncIndType(EncIndType encIndType) override;
};
