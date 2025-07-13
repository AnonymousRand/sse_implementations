#pragma once


#include "sse.h"
#include "utils/enc_ind.h"


/******************************************************************************/
/* `PiBasBase`                                                                */
/******************************************************************************/


// common code between `PiBas` and `PiBasLoc`
// note that we use the result-hiding variant of PiBas from figure 12 of Demertzis'20 (SDa paper) since SDa wants that
template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
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

    protected:
        ustring keyPrf;
        ustring keyEnc;

        virtual std::vector<DbDoc> searchBase(const Range<DbKw>& query) const = 0;
        ustring genQueryToken(const Range<DbKw>& query) const;

        /**
         * Returns:
         *     - `true` if the kv pair at `pos` is valid.
         *     - `false` if the kv pair at `pos` is the null kv pair.
         */
        virtual bool readEncIndValFromPos(ulong pos, std::pair<ustring, ustring>& ret) const = 0;
};


/******************************************************************************/
/* `PiBas`                                                                    */
/******************************************************************************/


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class PiBas : public PiBasBase<DbDoc, DbKw> {
    public:
        ~PiBas();

        // `ISse`
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        void clear() override;

    private:
        EncInd* encInd = new EncInd;

        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const override;
        bool readEncIndValFromPos(ulong pos, std::pair<ustring, ustring>& ret) const override;
};
