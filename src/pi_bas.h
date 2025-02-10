#pragma once

#include "sse.h"

template <typename KeyType = ustring, typename EncIndType = EncInd> // todo switch order of all these
class PiBasClient : public SseClient<KeyType, EncIndType> {
    protected:
        EncInd buildIndex(Db db);

    public:
        PiBasClient(Db db); // todo can replace with using?

        void setup(int secParam) override;
        EncIndType buildIndex() override;
        QueryToken trpdr(KwRange kwRange) override;
};

template <typename EncIndType = EncInd>
class PiBasServer : public SseServer<EncIndType> {
    public:
        std::vector<Id> search(QueryToken queryToken) override;
};
