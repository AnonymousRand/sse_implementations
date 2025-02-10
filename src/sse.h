// API for SSE schemes

#pragma once

#include <vector>

#include "util.h"

template <typename KeyType, typename EncIndType>
class SseClient {
    public:
        /**
         * Generate a key.
         *
         * `Setup()` from original paper broken up into `setup()` and `buildIndex()`
         * to be consistent with later papers.
         */
        virtual KeyType setup(int secParam) = 0;

        /**
         * Build the encrypted index.
         */
        virtual EncIndType buildIndex(KeyType key, Db db) = 0;

        /**
         * Issue a query by computing its encrypted token.
         */
        virtual QueryToken trpdr(KeyType key, KwRange kwRange) = 0;
};

template <typename EncIndType>
class SseServer {
    public:
        /**
         * Process a query and compute all results.
         */
        virtual std::vector<Id> search(EncIndType encInd, QueryToken queryToken) = 0;
};

template <typename KeyType, typename EncIndType>
class RangeSseClient : public SseClient<KeyType, EncIndType> {
    protected:
        SseClient<KeyType, EncIndType>& underlying;
};

template <typename EncIndType>
class RangeSseServer : public SseServer<EncIndType> {
    protected:
        SseServer<EncIndType>& underlying;
};
