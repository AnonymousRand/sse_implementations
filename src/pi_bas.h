#pragma once

#include "data_types.h"


class PiBasClient {
    private:
        Db db;
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
