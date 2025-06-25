#pragma once

#include "pi_bas.h"
#include "sse.h"


// don't use template template parameter for `Underly` because they may have other deeper underlying schemes
// and it can get very complicated so probably best to just explicitly specify everything here
template <ISdaUnderly_ Underly>
class Sda : public IDsse<Doc, Kw> {
    protected:
        std::vector<Underly*> underlys;
        EncIndType encIndType;
        int firstEmptyInd = 0;

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
