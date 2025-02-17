#pragma once

#include "util/util.h"

////////////////////////////////////////////////////////////////////////////////
// Client
////////////////////////////////////////////////////////////////////////////////

class PiBasClient {
    public:
        /**
         * Generate a key.
         *
         * `Setup()` from original paper broken up into `setup()` and `buildIndex()`
         * to be consistent with later papers.
         */
        ustring setup(int secParam);

        /**
         * Build the encrypted index.
         */
        template <typename DbDocType, typename DbKwType>
        EncInd buildIndex(const ustring& key, const Db<DbDocType, DbKwType>& db);

        /**
         * Issue a query by computing its encrypted token.
         */
        template <typename RangeType>
        QueryToken trpdr(const ustring& key, const Range<RangeType>& range);
};

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

class PiBasServer {
    public:
        /**
         * Process a query and compute all results.
         */
        template <typename DbDocType = Id>
        std::vector<DbDocType> search(const EncInd& encInd, const QueryToken& queryToken);
};
