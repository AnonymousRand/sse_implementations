#pragma once


#include "sse.h"
#include "utils/tdag.h"


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
class LogSrc : public ISdaUnderlySse<Doc, Kw> {
    public:
        LogSrc();
        ~LogSrc();

        void setup(int secParam, const Db<Doc, Kw>& db) override;
        std::vector<Doc> search(
            const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
        ) const override;

        void clear() override;
        Db<Doc, Kw> getDb() const override;
        long getSize() const override;

    private:
        Underly<Doc, Kw>* underly = nullptr;
        TdagNode<Kw>* tdag = nullptr;
        Db<Doc, Kw> db; // store instead of using underlying instance's `db` since that one has replications
};
