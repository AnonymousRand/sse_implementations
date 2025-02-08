#pragma once

#include "sse.h"

class PiBasClient : public SseClient {
    protected:
        EncIndex buildIndex(Db db);

    public:
        PiBasClient(Db db);

        void setup(int secParam) override;
        EncIndex buildIndex() override;
        QueryToken trpdr(KwRange kwRange) override;
};

class PiBasServer : public SseServer {
    public:
        std::vector<Id> search(QueryToken queryToken) override;
};
