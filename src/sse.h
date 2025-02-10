// API for SSE schemes

#pragma once

#include <vector>

#include "util.h"

template <typename KeyType, typename EncIndType>
class SseClient {
    protected:
        Db db;
        KeyType key;

    public:
        using BaseKeyType = KeyType;
        using BaseEncIndType = EncIndType;

        SseClient();
        SseClient(Db db);

        /**
         * Generate a key.
         *
         * `Setup()` from original paper broken up into `setup()` and `buildIndex()`
         * to be consistent with later papers.
         */
        virtual void setup(int secParam) = 0;

        /**
         * Build the encrypted index.
         */
        virtual EncIndType buildIndex() = 0;

        /**
         * Issue a query by computing its encrypted token.
         */
        virtual QueryToken trpdr(KwRange kwRange) = 0;
};

template <typename EncIndType>
class SseServer {
    protected:
        EncIndType encInd;

    public:
        using BaseEncIndType = EncIndType;

        /**
         * Process a query and compute all results.
         */
        virtual std::vector<Id> search(QueryToken queryToken) = 0;

        void setEncInd(EncIndType encInd);
        template <typename ClientType>
        std::vector<Id> query(ClientType& client, KwRange query);
};
