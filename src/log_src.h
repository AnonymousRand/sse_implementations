#pragma once

#include "pi_bas.h"

class LogSrcServer : public PiBasServer {
    public:
        std::vector<int> search(QueryToken queryToken);
};

class LogSrcClient : public PiBasClient {
    public:
        EncIndex buildIndex();
        QueryToken trpdr(KwRange kwRange);
};
