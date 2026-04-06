#pragma once


#include "sse.h"
#include "utils/enc_ind.h"


// note that we use the result-hiding variant of Pibas from figure 12 of NDSS'20 (SDa paper) since SDa wants that
template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
class Pibas : public ISdaUnderlySse<DbDoc, DbKw> {
    public:
        ~Pibas();

        //----------------------------------------------------------------------
        // `ISse`

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        std::vector<DbDoc> search(
            const Range<DbKw>& query, bool shouldCleanUpResults = true, bool isNaive = true
        ) const override;
        void clear() override;

        //----------------------------------------------------------------------
        // `ISdaUnderlySse`

        void getDb(Db<DbDoc, DbKw>& ret) const override;

    private:
        ustring encKey;
        ustring prfKey;
        EncInd* encInd = new EncInd();

        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const;
        ustring genQueryToken(const Range<DbKw>& query) const;

        /**
         * Generate encrypted label to store in encrypted index, and also return
         * numerical position at which to place it in the index (pseudorandomly).
         */
        ulong map(const ustring& queryToken, long dbKwListSize, ustring& retLabel) const;
};
