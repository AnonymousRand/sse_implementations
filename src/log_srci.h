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
        std::pair<EncInd, EncInd> buildIndex(const std::pair<ustring, ustring>& key, const Db<>& db);
        // interactivity messes up the API, otherwise we could've had a unified API for all SSE (anger)
        QueryToken trpdr1(const ustring& key1, const KwRange& kwRange);
        QueryToken trpdr2(const ustring& key2, const KwRange& kwRange, const std::vector<SrciDb1Doc>& choices);
};

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
class LogSrciServer : public IRangeSseServer<Underlying> {
    public:
        LogSrciServer(Underlying underlying);

        std::vector<SrciDb1Doc> search1(const EncInd& encInd1, const QueryToken& queryToken);
        std::vector<Id> search2(const EncInd& encInd2, const QueryToken& queryToken);
};
