#pragma once

#include "pi_bas.h"
#include "sse.h"

class LogSrciClient : public IRangeSseClient<std::pair<ustring, ustring>, std::pair<EncInd, EncInd>> {
    protected:
        TdagNode<Kw>* tdag1;
        TdagNode<Id>* tdag2;

    public:
        LogSrciClient(PiBasClient underlying);

        std::pair<ustring, ustring> setup(int secParam) override;
        std::pair<EncInd, EncInd> buildIndex(std::pair<ustring, ustring> key, Db<Id, KwRange> db) override;
        // interactivity messes up the API (anger)
        QueryToken trpdr1(ustring key1, KwRange kwRange);
        QueryToken trpdr2(ustring key2, std::vector<std::pair<KwRange, IdRange>> choices);
};

class LogSrciServer : public IRangeSseServer<std::pair<EncInd, EncInd>> {
    public:
        LogSrciServer(PiBasServer underlying);

        std::vector<std::pair<KwRange, IdRange>> search1(EncInd encInd1, QueryToken queryToken);
        std::vector<Id> search2(EncInd encInd2, QueryToken queryToken);
};
