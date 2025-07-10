#pragma once


#include "sse.h"


// don't use template template param for `Underly` because they may have other deeper underlying schemes 
// (e.g. `Sda<LogSrcI<PiBas>>`) and it gets complicated, so instead just specify all template params for `Underly` fully
template <IsSdaUnderlySse Underly>
class Sda : public IDsse<Doc, Kw> {
    public:
        ~Sda();

        void setup(int secParam, const Db<Doc, Kw>& db) override;
        void update(const DbEntry<Doc, Kw>& newDbEntry) override;
        std::vector<Doc> search(
            const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
        ) const override;

        void clear() override;

    private:
        std::vector<Underly*> underlys;
        long firstEmptyInd;
};
