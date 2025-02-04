#pragma once

#include "tree.h"
#include "pi_bas.h"


class LogSrcClient : public PiBasClient {
    public:
        EncIndex buildIndex();
        QueryToken trpdr(KwRange kwRange);
};


class LogSrcServer : public PiBasServer {
    public:
        std::vector<int> search(QueryToken queryToken);
};
