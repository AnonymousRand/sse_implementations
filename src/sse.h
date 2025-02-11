// APIs for SSE schemes

#pragma once

#include "util.h"

////////////////////////////////////////////////////////////////////////////////
// ISse
////////////////////////////////////////////////////////////////////////////////

template <typename KeyType, typename EncIndType>
class ISseClient {
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
        virtual EncIndType buildIndex(KeyType key, Db<Id, KwRange> db) = 0;

        /**
         * Issue a query by computing its encrypted token.
         */
        virtual QueryToken trpdr(KeyType key, KwRange kwRange) = 0;
};

template <typename EncIndType>
class ISseServer {
    public:
        /**
         * Process a query and compute all results.
         */
        virtual std::vector<Id> search(EncIndType encInd, QueryToken queryToken) = 0;
};

////////////////////////////////////////////////////////////////////////////////
// IRangeSse
////////////////////////////////////////////////////////////////////////////////

template <typename KeyType, typename EncIndType>
class IRangeSseClient : public ISseClient<KeyType, EncIndType> {
    protected:
        ISseClient<KeyType, EncIndType>& underlying;

    public:
        IRangeSseClient(ISseClient<KeyType, EncIndType>& underlying);
};

template <typename EncIndType>
class IRangeSseServer : public ISseServer<EncIndType> {
    protected:
        ISseServer<EncIndType>& underlying;

    public:
        IRangeSseServer(ISseServer<EncIndType>& underlying);
};
