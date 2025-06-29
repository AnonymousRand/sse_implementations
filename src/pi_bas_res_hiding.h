#pragma once


#include "sse.h"
#include "util/enc_ind.h"


/******************************************************************************/
/* `PiBasResHidingBase`                                                       */
/******************************************************************************/


template <IDbDoc_ DbDoc = Doc, class DbKw = Kw>
class PiBasResHidingBase : public ISdaUnderlySse<DbDoc, DbKw>, public IRangeUnderlySse<DbDoc, DbKw> {
    protected:
        Db<DbDoc, DbKw> db;
        ustring keyPrf;
        ustring keyEnc;
        IEncInd* encInd = nullptr;
        bool _isEmpty = false;

        ustring genQueryToken(const Range<DbKw>& query) const;

    public:
        PiBasResHidingBase() = default;
        PiBasResHidingBase(EncIndType encIndType);
        virtual ~PiBasResHidingBase();

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;

        std::vector<DbDoc> searchGeneric(const Range<DbKw>& query) const override;
        std::vector<DbDoc> searchAsRangeUnderlyGeneric(const Range<DbKw>& query) const;
        void clear() override;
        Db<DbDoc, DbKw> getDb() const override;
        bool isEmpty() const override;
        void setEncIndType(EncIndType encIndType) override;
};


/******************************************************************************/
/* `PiBasResHiding`                                                           */
/******************************************************************************/


template <IDbDoc_ DbDoc = Doc, class DbKw = Kw>
class PiBasResHiding : public PiBasResHidingBase<DbDoc, DbKw> {
    public:
        PiBasResHiding() = default;
        PiBasResHiding(EncIndType encIndType);

        std::vector<DbDoc> search(const Range<DbKw>& query) const override;

        std::vector<DbDoc> searchAsRangeUnderly(const Range<DbKw>& query) const override;
};


/******************************************************************************/
/* `PiBasResHiding` Template Specialization                                   */
/******************************************************************************/


template <>
class PiBasResHiding<Doc, Kw> : public PiBasResHidingBase<Doc, Kw> {
    public:
        PiBasResHiding() = default;
        PiBasResHiding(EncIndType encIndType);

        std::vector<Doc> search(const Range<Kw>& query) const override;

        std::vector<Doc> searchAsRangeUnderly(const Range<Kw>& query) const override;
};
