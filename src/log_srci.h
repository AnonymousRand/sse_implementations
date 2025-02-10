#pragma once

#include <unordered_map>

#include "sse.h"

using Ind1Val = std::pair<KwRange, IdRange>;

class LogSrciClient : public RangeSseClient<std::pair<ustring, ustring>, std::pair<EncInd, EncInd>> {
    protected:
        TdagNode* tdag1;
        TdagNode* tdag2;

    public:
        LogSrciClient(SseClient<ustring, EncInd>& underlying);

        std::pair<ustring, ustring> setup(int secParam) override;
        template <typename DbDocType, typename DbKwType>
        std::pair<EncInd, EncInd> buildIndex(std::pair<ustring, ustring> key, Db<DbDocType, DbKwType> db) override;
        // interactivity messes up the API (anger)
        QueryToken trpdr1(ustring key1, KwRange kwRange);
        QueryToken trpdr2(ustring key2, std::vector<Ind1Val> choices);
};

class LogSrciServer : public RangeSseServer<std::pair<EncInd, EncInd>> {
    public:
        LogSrciServer(SseServer<EncInd>& underlying);

        std::vector<Ind1Val> search1(EncInd encInd1, QueryToken queryToken);
        std::vector<Id> search2(EncInd encInd2, QueryToken queryToken);
};
