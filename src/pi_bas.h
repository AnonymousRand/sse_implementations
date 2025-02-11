#pragma once

#include "sse.h"

class PiBasClient : public ISseClient<ustring, EncInd> {
    public:
        ustring setup(int secParam) override;
        EncInd buildIndex(ustring key, Db<Id, KwRange> db) override;
        QueryToken trpdr(ustring key, KwRange kwRange) override;

        // "template functions cannot be virtual" wahh wahh why can't you be more like java you sadistic troglodyte
        template <typename DbDocType, typename DbKwType>
        EncInd buildIndexGeneric(ustring key, Db<DbDocType, DbKwType> db);
};

class PiBasServer : public ISseServer<EncInd> {
    public:
        std::vector<Id> search(EncInd encInd, QueryToken queryToken) override;
};
