#pragma once


#include "sse.h"
#include "utils/enc_ind.h"


// note that we use the result-hiding variant of PiBas from figure 12 of NDSS'20 (SDa paper) since SDa wants that
template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class PiBas : public IStaticPointSse<DbDoc, DbKw>, public ISdaUnderlySse<DbDoc, DbKw> {
    public:
        ~PiBas();

        //----------------------------------------------------------------------
        // `ISse`

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        void clear() override;

        //----------------------------------------------------------------------
        // `ISdaUnderlySse`

        void getDb(Db<DbDoc, DbKw>& ret) const override;

    private:
        PiBasServer<DbDoc, DbKw>* server = new PiBasServer();

        //----------------------------------------------------------------------
        // `IStaticPointSse`

        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const override;

        //----------------------------------------------------------------------
        // other

        ustring genQueryToken(const Range<DbKw>& query) const;
};
