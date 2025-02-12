#pragma once

#include "range_sse.h"
#include "util/tdag.h"

////////////////////////////////////////////////////////////////////////////////
// Client
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
class LogSrcClient : public IRangeSseClient<ustring, EncInd, Underlying> {
    protected:
        TdagNode<Kw>* tdag;

    public:
        LogSrcClient(Underlying underlying);

        ustring setup(int secParam) override;
        EncInd buildIndex(ustring key, Db<> db) override;
        QueryToken trpdr(ustring key, KwRange kwRange) override;
};

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
class LogSrcServer : public IRangeSseServer<Underlying> {
    public:
        LogSrcServer(Underlying underlying);

        std::vector<Id> search(EncInd encInd, QueryToken queryToken) override;
};
