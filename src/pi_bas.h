#pragma once

#include "util.h"

#include <unordered_set>

class PiBasServer {
    private:
        EncIndex encIndex;

    public:
        void setEncIndex(EncIndex encIndex);

        std::vector<int> search(QueryToken queryToken);
};

class PiBasClient {
    private:
        Db db;
        std::unordered_set<int> uniqueKws;
        ustring key;
        int keyLen;

    public:
        PiBasClient(Db db);
        ~PiBasClient();

        void setup(int secParam);
        EncIndex buildIndex();
        QueryToken trpdr(int kw);
};
