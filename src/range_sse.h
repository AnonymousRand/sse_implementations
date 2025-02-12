#pragma once

#include "pi_bas.h"
#include "sse.h"

////////////////////////////////////////////////////////////////////////////////
// Client
////////////////////////////////////////////////////////////////////////////////

template <typename KeyType, typename EncIndType, typename Underlying>
class IRangeSseClient : public ISseClient<KeyType, EncIndType> {
    protected:
        Underlying underlying;

    public:
        IRangeSseClient(Underlying underlying);
};

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
class IRangeSseServer : public ISseServer {
    protected:
        Underlying underlying;

    public:
        IRangeSseServer(Underlying underlying);
};
