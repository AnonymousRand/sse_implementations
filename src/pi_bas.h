#pragma once

#include "util.h"

#include <unordered_set>

class PiBasServer {
    private:
        EncIndex encIndex;

    public:
        void setEncIndex(EncIndex encIndex);

        /**
         * Process a query and compute all results.
         */
        std::vector<int> search(QueryToken queryToken);
};

class PiBasClient {
    private:
        Db db;
        std::unordered_set<int> uniqueKws;
        ustring key;
        int keyLen;

    public:
        PiBasClient(Db db);

        /**
         * Generate a key.
         *
         * `Setup()` from original paper broken up into `setup()` and `buildIndex()`
         * to be consistent with later papers.
         */
        void setup(int secParam);

        /**
         * Build the encrypted index.
         */
        EncIndex buildIndex();

        /**
         * Issue a query by computing its encrypted token.
         */
        QueryToken trpdr(int kw);
};
