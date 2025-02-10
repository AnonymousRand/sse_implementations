#pragma once

#include "sse.h"

class PiBasClient : public SseClient<ustring, EncInd> {
    public:
        ustring setup(int secParam) override;
        template <typename DbDocType, typename DbKwType>
        EncInd buildIndex(ustring key, Db<DbDocType, DbKwType> db) override;
        QueryToken trpdr(ustring key, KwRange kwRange) override;
};

class PiBasServer : public SseServer<EncInd> {
    public:
        std::vector<Id> search(EncInd encInd, QueryToken queryToken) override;
};
