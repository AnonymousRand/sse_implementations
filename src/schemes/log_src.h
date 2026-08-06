#pragma once

#include "schemes/sse.h"
#include "utils/tdag.h"


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
class LogSrc : public ISdaUnderlySse<Doc<>, Kw> {
    public:
        using ISdaUnderlySse<Doc<>, Kw>::ISdaUnderlySse;

        ~LogSrc();

        //----------------------------------------------------------------------
        // `ISse`

        void setup(int secParam, const Db<Doc<>, Kw>& db) override;
        std::vector<Doc<>> search(
            const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
        ) const override;
        void clear() override;

        //----------------------------------------------------------------------
        // `ISdaUnderlySse`

        void getDb(Db<Doc<>, Kw>& ret) const override;

    private:
        Underly<Doc<>, Kw>* underly = new Underly<Doc<>, Kw> {this->benchmark};
        TdagNode<Kw>* tdag = nullptr;
};
