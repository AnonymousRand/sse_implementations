#pragma once

#include "pi_bas.h"
#include "range_sse.h"

////////////////////////////////////////////////////////////////////////////////
// Client
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
class LogSrciClient : public IRangeSseClient<std::pair<ustring, ustring>, std::pair<EncInd, EncInd>, Underlying> {
    protected:
        TdagNode<Kw>* tdag1;
        TdagNode<Id>* tdag2;

    public:
        LogSrciClient(Underlying underlying);

        std::pair<ustring, ustring> setup(int secParam) override;
        std::pair<EncInd, EncInd> buildIndex(std::pair<ustring, ustring> key, Db<> db) override;
        // interactivity messes up the API (anger)
        QueryToken trpdr(ustring key1, KwRange kwRange) override;
        QueryToken trpdr2(ustring key2, std::vector<SrciDb1Doc> choices);
};

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
class LogSrciServer : public IRangeSseServer<Underlying> {
    public:
        LogSrciServer(Underlying underlying);

        std::vector<SrciDb1Doc> search1(EncInd encInd1, QueryToken queryToken);
        std::vector<Id> search(EncInd encInd2, QueryToken queryToken) override;
};
