#pragma once

#include "sse.h"
#include "tdag.h"

class LogSrcClient : public RangeSseClient<ustring, EncInd> {
    protected:
        TdagNode* tdag;

    public:
        LogSrcClient();

        EncInd buildIndex(ustring key, Db db) override;
        QueryToken trpdr(ustring key, KwRange kwRange) override;
};

class LogSrcServer : public RangeSseServer<EncInd> {};
