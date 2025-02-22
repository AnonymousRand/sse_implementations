#pragma once

#include "sse.h"

template <class DbDoc = IdOp, class DbKw = Kw>
class PiBas : public ISse<DbDoc, DbKw> {
    private:
        ustring key;
        EncInd encInd;

    public:
        // API functions
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;

        // non-API functions
        void genKey(int secParam);
        void buildIndex(const Db<DbDoc, DbKw>& db);
        QueryToken genQueryToken(const Range<DbKw>& query) const;
        std::vector<DbDoc> genericSearch(const QueryToken& queryToken) const;
        //std::vector<IdOp> searchWithDels(const QueryToken& queryToken) const;
};
