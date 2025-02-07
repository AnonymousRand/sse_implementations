#pragma once

#include "util.h"

#include <unordered_set>

class SseServer {
    protected:
        EncIndex encIndex;

    public:
        void setEncIndex(EncIndex encIndex);

        /**
         * Process a query and compute all results.
         */
        virtual std::vector<int> search(QueryToken queryToken) = 0;
};

class SseClient {
    protected:
        Db db;
        std::unordered_set<int> uniqueKws; // used during `setup()`
        ustring key;
        int keyLen;

    public:
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
        virtual EncIndex buildIndex() = 0;

        /**
         * Issue a query by computing its encrypted token.
         */
        virtual QueryToken trpdr(int kw) = 0;
};
