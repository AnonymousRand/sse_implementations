#pragma once

#include "data_types.h"

class PiBasClient {
    private:
        Db db;

    public:
        std::string setup(int secParam);
        EncIndex buildIndex(std::string key, Db db);
        QueryToken trpdr(std::string key, Kw kw);
};

class PiBasServer {
    private:
        EncIndex encIndex;

    public:
        std::vector<Id> search(QueryToken queryToken);
};
