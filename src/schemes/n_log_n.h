#pragma once

#include "n_log_n_server.h"
#include "sse.h"
#include "utils/benchmark.h" // since this was only forward declared in `sse.h`
#include "utils/enc_ind.h"


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class NLogN : public IStaticPointSse<DbDoc, DbKw>, public ISdaUnderlySse<DbDoc, DbKw> {
    public:
        using IStaticPointSse<DbDoc, DbKw>::IStaticPointSse;

        ~NLogN();

        //----------------------------------------------------------------------
        // `ISse`

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        void clear() override;

        //----------------------------------------------------------------------
        // `ISdaUnderlySse`

        void getDb(Db<DbDoc, DbKw>& ret) const override;

    protected:
        NLogNServer<DbDoc, DbKw>* server = new NLogNServer<DbDoc, DbKw> {this->benchmark};
        long numLvls;

        //----------------------------------------------------------------------
        // `IStaticPointSse`

        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const override;

        //----------------------------------------------------------------------
        // other

        ustring genQueryToken(const Range<DbKw>& query) const;

        /**
         * Generate encrypted label to store in encrypted index, and also return
         * numerical position only at which to place it in the index with no modulus for bucket count.
         */
        ulong mapNoMod(const ustring& queryToken, ustring& retLabel) const;

        /**
         * Generate encrypted label to store in encrypted index, and also return
         * numerical level and position at which to place it in the index.
         * (Position is a bucket count, not entry count, so it is the raw position modulo bucket count on that level.)
         *
         * Preconditions:
         *     - `dbKwListSize` is a power of 2.
         */
        std::pair<ulong, ulong> map(const ustring& queryToken, long dbKwListSize, ustring& retLabel) const;

        virtual long computeNumLvls() const;
        virtual long computeBcktCountOnLvl(long lvlNum) const;
        virtual long computeBcktSizeOnLvl(long lvlNum) const;
};
