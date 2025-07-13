#pragma once


#include "sse.h"
#include "utils/tdag.h"


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
class LogSrc : public ISdaUnderlySse<Doc<>, Kw> {
    public:
        LogSrc();
        ~LogSrc();

        // `ISse`
        void setup(int secParam, const Db<Doc<>, Kw>& db) override;
        std::vector<Doc<>> search(
            const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
        ) const override;
        void clear() override;

        // `ISdaUnderlySse`
        Db<Doc<>, Kw> getDb() const override;

    private:
        Underly<Doc<>, Kw>* underly = nullptr;
        TdagNode<Kw>* tdag = nullptr;
};
