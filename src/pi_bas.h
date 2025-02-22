#pragma once

#include "sse.h"

// >todo make sure everything works with id instead of doc, as is the point of templates
template <class DbDoc = Doc, class DbKw = Kw>
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
        //std::vector<Doc> searchWithDels(const QueryToken& queryToken) const;
};
