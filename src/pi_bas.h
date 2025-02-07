#pragma once

#include "util.h"

#include <unordered_set>

class PiBasServer {
    public:
        EncIndex encIndex;
        std::vector<int> search(QueryToken queryToken);
};

class PiBasClient {
    private:
        Db db;
        std::unordered_set<int> uniqueKws;
        unsigned char* key;
        int secParam;

    public:
        PiBasClient(Db db);
        void sendEncIndex(PiBasServer server, EncIndex encIndex);

        void setup(int secParam);
        EncIndex buildIndex();
        QueryToken trpdr(int kw);
};
