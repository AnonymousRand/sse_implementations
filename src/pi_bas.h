#pragma once

#include "sse.h"

template <typename DbDoc = IdOp>
class PiBas : ISse<DbDoc> {
    private:
        ustring key;
        EncInd encInd;

    public:
        // API functions
        
        void setup(int secParam, const Db<DbDoc>& db) override;
        std::vector<DbDoc> search(const KwRange& query) override;

        // non-API functions
        
        ustring genKey(int secParam) const;

        template <typename DbDoc2, typename DbKw>
        EncInd buildIndex(const ustring& key, const Db<DbDoc2, DbKw>& db) const;

        template <typename RangeType>
        QueryToken genQueryToken(const ustring& key, const Range<RangeType>& range) const;

        template <typename DbDoc2 = DbDoc>
        std::vector<DbDoc2> serverSearch(const EncInd& encInd, const QueryToken& queryToken) const;
};
