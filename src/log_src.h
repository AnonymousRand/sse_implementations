#pragma once

#include "sse.h"

class LogSrcClient : public RangeSseClient<ustring, EncInd> {
    protected:
        TdagNode* tdag;

    public:
        LogSrcClient(SseClient<ustring, EncInd>& underlying);

        ustring setup(int secParam) override;
        EncInd buildIndex(ustring key, Db db) override;
        QueryToken trpdr(ustring key, KwRange kwRange) override;
};

class LogSrcServer : public RangeSseServer<EncInd> {
    public:
        LogSrcServer(SseServer<EncInd>& underlying);

        std::vector<Id> search(EncInd encInd, QueryToken queryToken) override;
};
