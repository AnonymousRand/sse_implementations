#pragma once

#include "range_sse.h"
#include "util/tdag.h"

////////////////////////////////////////////////////////////////////////////////
// Client
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
class LogSrcClient : public IRangeSseClient<Underlying> {
    protected:
        const TdagNode<Kw>* tdag;

    public:
        LogSrcClient(Underlying underlying);

        ustring setup(int secParam);
        EncInd buildIndex(ustring key, Db<> db);
        QueryToken trpdr(ustring key, KwRange kwRange);
};

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
class LogSrcServer : public IRangeSseServer<Underlying> {
    public:
        LogSrcServer(Underlying underlying);

        std::vector<Id> search(EncInd encInd, QueryToken queryToken);
};
