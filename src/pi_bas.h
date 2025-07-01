#pragma once


#include "sse.h"


/******************************************************************************/
/* `PiBasBase`                                                                */
/******************************************************************************/


// common code between `PiBas` and `PiBasLoc`
// note that we use the result-hiding variant of PiBas since SDa wants that iirc
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
        PiBasBase() = default;
        PiBasBase(EncIndType encIndType);
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
        IEncInd* encInd = nullptr;

        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const override;

    public:
        PiBas() = default;
        PiBas(EncIndType encIndType);
        ~PiBas();

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;

        void clear() override;
        void setEncIndType(EncIndType encIndType) override;
};


/******************************************************************************/
/* `PiBasLoc`                                                                 */
/******************************************************************************/


template <IDbDoc_ DbDoc = Doc, class DbKw = Kw>
class PiBasLoc : public PiBasBase<DbDoc, DbKw> {
    private:
        IEncIndLoc* encInd = nullptr;

        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const override;

    public:
        PiBasLoc() = default;
        PiBasLoc(EncIndType encIndType);
        ~PiBasLoc();

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;

        void clear() override;
        void setEncIndType(EncIndType encIndType) override;
};
