#pragma once


#include "sse.h"


// don't use template template param for `Underly` because they may have other deeper underlying schemes 
// (e.g. `Sda<LogSrcI<PiBas>>`) and it gets complicated, so instead just specify all template params for `Underly` fully
template <IsSdaUnderly Underly>
class Sda : public IDsse<Doc, Kw> {
    protected:
        std::vector<Underly*> underlys;
        long firstEmptyInd = 0;

    public:
        ~Sda();

        void setup(int secParam, const Db<Doc, Kw>& db) override;
        std::vector<Doc> search(
            const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
        ) const override;
        void update(const DbEntry<Doc, Kw>& newEntry) override;

        void clear() override;
};
