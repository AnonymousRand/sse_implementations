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
        EncInd buildIndex(ustring key, Db<> db);

        /**
         * Issue a query by computing its encrypted token.
         */
        QueryToken trpdr(ustring key, KwRange kwRange);

        // non-API functions for SSEs building on top of PiBas and requiring more polymorphic types
        // "template functions cannot be virtual" wahh wahh why can't you be more like java you sadistic troglodyte
        template <typename DbDocType, typename DbKwType>
        EncInd buildIndexGeneric(ustring key, Db<DbDocType, DbKwType> db);
        template <typename RangeType>
        QueryToken trpdrGeneric(ustring key, Range<RangeType> range);
};

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

class PiBasServer {
    public:
        /**
         * Process a query and compute all results.
         */
        virtual std::vector<Id> search(EncInd encInd, QueryToken queryToken);

        // non-API
        template <typename DbDocType = Id>
        std::vector<DbDocType> searchGeneric(EncInd encInd, QueryToken queryToken);
};
