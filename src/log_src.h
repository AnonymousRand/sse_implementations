#pragma once

#include "sse.h"

class LogSrcClient : public IRangeSseClient<ustring, EncInd> {
    protected:
        TdagNode* tdag;

    public:
        LogSrcClient(ISseClient<ustring, EncInd>& underlying);

        ustring setup(int secParam) override;
        EncInd buildIndex(ustring key, Db db) override;
        QueryToken trpdr(ustring key, KwRange kwRange) override;
};

class LogSrcServer : public IRangeSseServer<EncInd> {
    public:
        LogSrcServer(ISseServer<EncInd>& underlying);

        std::vector<Id> search(EncInd encInd, QueryToken queryToken) override;
};
