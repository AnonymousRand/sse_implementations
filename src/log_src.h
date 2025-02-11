#pragma once

#include "sse.h"

class LogSrcClient : public IRangeSseClient<ustring, EncInd> {
    protected:
        TdagNode<Kw>* tdag;

    public:
        LogSrcClient(ISseClient<ustring, EncInd>& underlying);

        ustring setup(int secParam) override;
        EncInd buildIndex(ustring key, Db<Id, KwRange> db) override;
        QueryToken trpdr(ustring key, KwRange kwRange) override;
};

class LogSrcServer : public IRangeSseServer<EncInd> {
    public:
        LogSrcServer(PiBasClient<EncInd>& underlying);

        std::vector<Id> search(EncInd encInd, QueryToken queryToken) override;
};
