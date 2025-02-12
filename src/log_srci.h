#pragma once

#include "range_sse.h"
#include "util/tdag.h"

////////////////////////////////////////////////////////////////////////////////
// Client
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
class LogSrciClient : public IRangeSseClient<Underlying> {
    protected:
        TdagNode<Kw>* tdag1;
        TdagNode<Id>* tdag2;

    public:
        LogSrciClient(Underlying underlying);

        std::pair<ustring, ustring> setup(int secParam);
        std::pair<EncInd, EncInd> buildIndex(std::pair<ustring, ustring> key, Db<> db);
        // interactivity messes up the API, otherwise we could've had a unified API for all SSE (anger)
        QueryToken trpdr1(ustring key1, KwRange kwRange);
        QueryToken trpdr2(ustring key2, KwRange kwRange, std::vector<SrciDb1Doc> choices);
};

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
class LogSrciServer : public IRangeSseServer<Underlying> {
    public:
        LogSrciServer(Underlying underlying);

        std::vector<SrciDb1Doc> search1(EncInd encInd1, QueryToken queryToken);
        std::vector<Id> search2(EncInd encInd2, QueryToken queryToken);
};
