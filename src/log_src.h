#pragma once

#include "range_sse.h"
#include "util/tdag.h"

////////////////////////////////////////////////////////////////////////////////
// Client
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
class LogSrcClient : public IRangeSseClient<Underlying> {
    protected:
        TdagNode<Kw>* tdag;

    public:
        LogSrcClient(Underlying underlying);

        ustring setup(int secParam);
        EncInd buildIndex(const ustring& key, const Db<>& db);
        QueryToken trpdr(const ustring& key, const KwRange& kwRange);
};

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
class LogSrcServer : public IRangeSseServer<Underlying> {
    public:
        LogSrcServer(Underlying underlying);

        std::vector<Id> search(const EncInd& encInd, const QueryToken& queryToken);
};
