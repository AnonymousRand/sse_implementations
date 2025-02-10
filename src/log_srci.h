#pragma once

#include <unordered_map>

#include "sse.h"

typedef std::pair<KwRange, IdRange> Ind1Val;
typedef std::unordered_map<KwRange, std::vector<Ind1Val>> Ind1;

class LogSrciClient : public RangeSseClient<std::pair<ustring, ustring>, std::pair<EncInd, EncInd>> {
    protected:
        // have to store index for ind1 since TDAG alone can no longer handle its data format in its nodes
        Ind1 ind1; 
        TdagNode* tdag1;
        TdagNode* tdag2;

    public:
        LogSrciClient(SseClient<ustring, EncInd>& underlying);

        std::pair<ustring, ustring> setup(int secParam) override;
        std::pair<EncInd, EncInd> buildIndex(std::pair<ustring, ustring> key, Db db) override;
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
