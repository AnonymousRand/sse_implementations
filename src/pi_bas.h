#pragma once


#include "sse.h"
#include "util/enc_ind.h"


/******************************************************************************/
/* `PiBasBase`                                                                */
/******************************************************************************/


// common code between `PiBas` and `PiBasLoc`
// note that we use the result-hiding variant of PiBas from figure 12 of Demertzis'20 (SDa paper) since SDa wants that
template <IDbDoc_ DbDoc, class DbKw>
class PiBasBase : public ISse<DbDoc, DbKw>, public ISdaUnderly<DbDoc, DbKw> {
    protected:
        ustring keyPrf;
        ustring keyEnc;
        Db<DbDoc, DbKw> db;
        bool _isEmpty = false;

        virtual std::vector<DbDoc> searchBase(const Range<DbKw>& query) const = 0;
        ustring genQueryToken(const Range<DbKw>& query) const;

    public:
        virtual ~PiBasBase() = default;

        std::vector<DbDoc> search(
            const Range<DbKw>& query, bool shouldCleanUpResults = true, bool isNaive = true
        ) const override;

        void clear() override;
        Db<DbDoc, DbKw> getDb() const override;
        bool isEmpty() const override;
};


/******************************************************************************/
/* `PiBas`                                                                    */
/******************************************************************************/


template <IDbDoc_ DbDoc = Doc, class DbKw = Kw>
class PiBas : public PiBasBase<DbDoc, DbKw> {
    private:
        EncInd* encInd = new EncInd;

        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const override;

    public:
        ~PiBas();

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;

        void clear() override;
};
