#pragma once

#include "util/util.h"

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

class ISseServer {
    public:
        /**
         * Process a query and compute all results.
         */
        virtual std::vector<Id> search(const EncInd& encInd, const QueryToken& queryToken) = 0;
};

////////////////////////////////////////////////////////////////////////////////
// Client
////////////////////////////////////////////////////////////////////////////////

class ISseClient {
    public:
        /**
         * Generate a key.
         *
         * `Setup()` from original paper broken up into `setup()` and `buildIndex()`
         * to be consistent with later papers.
         */
        virtual ustring setup(int secParam) = 0;

        /**
         * Build the encrypted index.
         */
        virtual EncInd buildIndex(const ustring& key, const Db<>& db) = 0;

        /**
         * Issue a query by computing its encrypted token and return the results.
         */
        virtual QueryToken query(const ustring& key, const KwRange& range, const ISseServer& server) = 0;
};
