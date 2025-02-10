#pragma once

#include "pi_bas.h"
#include "tdag.h"

class LogSrcClient : public PiBasClient<ustring, EncInd> {
    protected:
        TdagNode* tdag;

    public:
        using Super = PiBasClient<ustring, EncInd>;

        LogSrcClient(Db db);

        BaseEncIndType buildIndex() override;
        QueryToken trpdr(KwRange kwRange) override;
};

class LogSrcServer : public PiBasServer<EncInd> {};
