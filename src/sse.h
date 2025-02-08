#pragma once

#include <vector>

#include "util.h"

class SseClient {
    protected:
        Db db;
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
        virtual QueryToken trpdr(KwRange kwRange) = 0;
};

class SseServer {
    protected:
        EncIndex encIndex;

    public:
        /**
         * Process a query and compute all results.
         */
        virtual std::vector<Id> search(QueryToken queryToken) = 0;

        void setEncIndex(EncIndex encIndex);
};
