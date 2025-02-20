#pragma once

#include "sse.h"

template <typename DbDocType = Id>
class PiBas : ISse<DbDocType> {
    private:
        ustring key;
        EncInd encInd;

    public:
        // API functions
        
        void setup(int secParam, const Db<DbDocType>& db) override;
        std::vector<DbDocType> search(const KwRange& query) override;

        // non-API functions
        
        ustring genKey(int secParam) const;

        template <typename DbDocType2, typename DbKwType>
        EncInd buildIndex(const ustring& key, const Db<DbDocType2, DbKwType>& db) const;

        template <typename RangeType>
        QueryToken genQueryToken(const ustring& key, const Range<RangeType>& range) const;

        template <typename DbDocType2>
        std::vector<DbDocType2> serverSearch(const EncInd& encInd, const QueryToken& queryToken) const;
};
