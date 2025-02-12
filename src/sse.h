// APIs for SSE schemes

#pragma once

#include "util/util.h"

////////////////////////////////////////////////////////////////////////////////
// Client
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
        virtual EncIndType buildIndex(KeyType key, Db<> db) = 0;

        /**
         * Issue a query by computing its encrypted token.
         */
        virtual QueryToken trpdr(ustring key, KwRange kwRange) = 0;
};

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

class ISseServer {
    public:
        /**
         * Process a query and compute all results.
         */
        virtual std::vector<Id> search(EncInd encInd, QueryToken queryToken) = 0;
};
