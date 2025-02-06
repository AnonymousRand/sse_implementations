#pragma once

#include "data_types.h"

#include <unordered_set>


class PiBasClient {
    private:
        Db db;
        std::unordered_set<int> uniqueKws;
        unsigned char* key;

    public:
        PiBasClient(Db db);

        void setup(int secParam);
        EncIndex buildIndex();
        QueryToken trpdr(int kw);
};


class PiBasServer {
    private:
        EncIndex encIndex;

    public:
        std::vector<int> search(QueryToken queryToken);
};
