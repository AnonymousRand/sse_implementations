#pragma once


#include "sse.h"


/******************************************************************************/
/* `PiBasBase`                                                                */
/******************************************************************************/


// common code between `PiBas` and `PiBasResHiding`
template <IDbDoc_ DbDoc, class DbKw>
class PiBasBase : public ISse<DbDoc, DbKw>, public ISdaUnderly<DbDoc, DbKw> {
    protected:
        Db<DbDoc, DbKw> db;
        IEncInd* encInd = nullptr;
        bool _isEmpty = false;

        virtual std::vector<DbDoc> searchBase(const Range<DbKw>& query) const = 0;

    public:
        PiBasBase() = default;
        PiBasBase(EncIndType encIndType);
        virtual ~PiBasBase();

        std::vector<DbDoc> search(
            const Range<DbKw>& query, bool shouldCleanUpResults = true, bool isNaive = true
        ) const override;

        void clear() override;
        Db<DbDoc, DbKw> getDb() const override;
        bool isEmpty() const override;
        void setEncIndType(EncIndType encIndType) override;
};


template <IDbDoc_ DbDoc = Doc, class DbKw = Kw>
class PiBas : public PiBasBase<DbDoc, DbKw> {
    private:
        ustring key;

        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const override;
        std::pair<ustring, ustring> genQueryToken(const Range<DbKw>& query) const;

    public:
        PiBas() = default;
        PiBas(EncIndType encIndType);

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
};


/******************************************************************************/
/* `PiBasResHiding`                                                           */
/******************************************************************************/


template <IDbDoc_ DbDoc = Doc, class DbKw = Kw>
class PiBasResHiding : public PiBasBase<DbDoc, DbKw> {
    private:
        ustring keyPrf;
        ustring keyEnc;

        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const override;
        ustring genQueryToken(const Range<DbKw>& query) const;

    public:
        PiBasResHiding() = default;
        PiBasResHiding(EncIndType encIndType);

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
};
