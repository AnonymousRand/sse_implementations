#pragma once

#include "sse.h"

class PiBas : ISse {
    private:
        ustring key;
        EncInd encInd;

    public:
        // API functions
        
        void setup(int secParam, const Db<>& db) override;
        std::vector<Doc> search(const KwRange& query) override;

        // non-API functions
        
        ustring genKey(int secParam) const;

        template <typename DbDoc, typename DbKw>
        EncInd buildIndex(const ustring& key, const Db<DbDoc, DbKw>& db) const;

        template <typename RangeType>
        QueryToken genQueryToken(const ustring& key, const Range<RangeType>& range) const;

        template <typename DbDoc = Doc>
        std::vector<DbDoc> serverSearch(const EncInd& encInd, const QueryToken& queryToken) const;
};
