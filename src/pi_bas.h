#pragma once


#include "sse.h"
#include "utils/enc_ind.h"


/******************************************************************************/
/* `PiBasBase`                                                                */
/******************************************************************************/


// common code between `PiBas` and `PiBasLoc`
// note that we use the result-hiding variant of PiBas from figure 12 of Demertzis'20 (SDa paper) since SDa wants that
template <IsDbDOc DbDoc, class DbKw>
class PiBasBase : public ISdaUnderlySse<DbDoc, DbKw> {
    public:
        virtual ~PiBasBase() = default;

        // `ISse`
        std::vector<DbDoc> search(
            const Range<DbKw>& query, bool shouldCleanUpResults = true, bool isNaive = true
        ) const override;
        void clear() override;

        // `ISdaUnderlySse`
        Db<DbDoc, DbKw> getDb() const override;
        long getSize() const override;

    protected:
        ustring keyPrf;
        ustring keyEnc;
        Db<DbDoc, DbKw> db;
        long size;

        virtual std::vector<DbDoc> searchBase(const Range<DbKw>& query) const = 0;
        ustring genQueryToken(const Range<DbKw>& query) const;
};


/******************************************************************************/
/* `PiBas`                                                                    */
/******************************************************************************/


template <IsDbDOc DbDoc = Doc, class DbKw = Kw>
class PiBas : public PiBasBase<DbDoc, DbKw> {
    public:
        ~PiBas();

        // `ISse`
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        void clear() override;

    private:
        EncInd* encInd = new EncInd;

        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const override;
};
