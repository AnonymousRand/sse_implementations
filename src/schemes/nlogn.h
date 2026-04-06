#pragma once


#include "sse.h"
#include "utils/enc_ind.h"


/*
todo brainstorming: for log-src-i* underly, just subclass this and override setup() & map() with now each level
storing the custom amount of buckets/entries based on # of tdag nodes on that level 
and for the size param pass in the number of leaves
also remove padding right
change private here to protected
*/
template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
class Nlogn : public IStaticPointSse<DbDoc, DbKw>, public ISdaUnderlySse<DbDoc, DbKw> {
    public:
        ~Nlogn();

        //----------------------------------------------------------------------
        // `ISse`

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        void clear() override;

        //----------------------------------------------------------------------
        // `ISdaUnderlySse`

        void getDb(Db<DbDoc, DbKw>& ret) const override;

    private:
        std::vector<EncInd*> encIndLvls;
        EncInd* dbKwListSizeDict = new EncInd();

        //----------------------------------------------------------------------
        // `IStaticPointSse`

        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const override;

        //----------------------------------------------------------------------
        // other

        ustring genQueryToken(const Range<DbKw>& query) const;

        /**
         * Generate encrypted label to store in encrypted index, and also return
         * numerical level & position at which to place it in the index (in a locality-preserving matter:
         * only randomness is which bucket to choose in the level, and also `pos` will be aligned to buckets).
         *
         * Preconditions:
         *     - `dbKwListSize` is a power of 2.
         */
        std::pair<ulong, ulong> map(const ustring& queryToken, long dbKwListSize, ustring& retLabel) const;

        /**
         * Generate encrypted label to store in encrypted index, and also return
         * numerical position only at which to place it in the index.
         * Use for looking up `dbKwListSizeDict`; no rounding.
         */
        ulong mapForDict(const ustring& queryToken, ustring& retLabel) const;
};
