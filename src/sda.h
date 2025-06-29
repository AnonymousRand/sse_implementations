#pragma once


#include "pi_bas.h"
#include "sse.h"


// don't use template template param for `Underly` because they may have other deeper underlying schemes 
// (e.g. `Sda<LogSrcI<PiBas>>`) and it gets complicated, so instead just specify all template params for `Underly` fully
template <ISdaUnderlySse_ Underly>
class Sda : public IDsse<Doc, Kw> {
    protected:
        std::vector<Underly*> underlys;
        EncIndType encIndType;
        ulong firstEmptyInd = 0;

        std::vector<Doc> searchWithoutRemovingDels(const Range<Kw>& query) const;

    public:
        Sda(EncIndType encIndType);
        ~Sda();

        void setup(int secParam, const Db<Doc, Kw>& db) override;
        std::vector<Doc> search(const Range<Kw>& query) const;
        void update(const DbEntry<Doc, Kw>& newEntry) override;

        void clear() override;
        void setEncIndType(EncIndType encIndType) override;
};
