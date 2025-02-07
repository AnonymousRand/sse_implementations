#pragma once

#include "sse.h"

class PiBasServer : public SseServer {
    public:
        std::vector<Id> search(QueryToken queryToken);
};

class PiBasClient : public SseClient {
    public:
        PiBasClient(Db db);

        void setup(int secParam);
        EncIndex buildIndex();
        QueryToken trpdr(KwRange kwRange);
};
