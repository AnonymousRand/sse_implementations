#pragma once

#include "node.h"
#include "pi_bas.h"

class LogSrcClient : public PiBasClient {
    public:
        EncIndex buildIndex(std::string key, Db db);
        QueryToken trpdr(std::string key, KwRange kwRange);
};

class LogSrcServer : public PiBasServer {
    public:
        std::vector<Id> search(QueryToken queryToken);
};
