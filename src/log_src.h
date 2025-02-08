#pragma once

#include "pi_bas.h"
#include "tdag.h"

class LogSrcClient : public PiBasClient {
    protected:
        TdagNode* tdag;

    public:
        LogSrcClient(Db db);

        EncIndex buildIndex() override;
        QueryToken trpdr(KwRange kwRange);
};

class LogSrcServer : public PiBasServer {
    public:
        std::vector<Id> search(QueryToken queryToken);
};
