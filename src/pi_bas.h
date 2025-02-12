#pragma once

#include "sse.h"

////////////////////////////////////////////////////////////////////////////////
// Client
////////////////////////////////////////////////////////////////////////////////

class PiBasClient : public ISseClient<ustring, EncInd> {
    public:
        ustring setup(int secParam) override;
        EncInd buildIndex(ustring key, Db<> db) override;
        QueryToken trpdr(ustring key, KwRange kwRange) override;

        // non-API functions for SSEs building on top of PiBas and requiring more polymorphic types
        // "template functions cannot be virtual" wahh wahh why can't you be more like java you sadistic troglodyte
        template <typename DbDocType, typename DbKwType>
        EncInd buildIndexGeneric(ustring key, Db<DbDocType, DbKwType> db);
        template <typename RangeType>
        QueryToken trpdrGeneric(ustring key, Range<RangeType> range);
};

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

class PiBasServer : public ISseServer {
    public:
        std::vector<Id> search(EncInd encInd, QueryToken queryToken) override;

        template <typename DbDocType = Id>
        std::vector<DbDocType> searchGeneric(EncInd encInd, QueryToken queryToken);
};
