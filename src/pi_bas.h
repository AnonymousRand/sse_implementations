#pragma once

#include "sse.h"
#include "util/enc_ind.h"

/******************************************************************************/
/* PiBas                                                                      */
/******************************************************************************/

template <IDbDoc_ DbDoc, class DbKw>
class PiBasBase : public ISdaUnderly<DbDoc, DbKw> {
    protected:
        Db<DbDoc, DbKw> db;
        ustring key;
        IEncInd* encInd = nullptr;
        bool _isEmpty = false;

        std::pair<ustring, ustring> genQueryToken(const Range<DbKw>& query) const;

    public:
        PiBasBase() = default;
        PiBasBase(EncIndType encIndType);
        virtual ~PiBasBase(); // must be `virtual` for compiler to not scream about undefined ref to child destructor

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;

        std::vector<DbDoc> searchWithoutRemovingDels(const Range<DbKw>& query) const override;
        void clear() override;
        Db<DbDoc, DbKw> getDb() const override;
        bool isEmpty() const override;
        void setEncIndType(EncIndType encIndType) override;
};

// this is literally just so I can partially specialize `search()`'s template for when `DbDoc` is `Doc`
template <IDbDoc_ DbDoc = Doc, class DbKw = Kw>
class PiBas : public PiBasBase<DbDoc, DbKw> {
    public:
        PiBas() = default;
        PiBas(EncIndType encIndType);

        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};

template <>
class PiBas<Doc, Kw> : public PiBasBase<Doc, Kw> {
    public:
        PiBas() = default;
        PiBas(EncIndType encIndType);

        std::vector<Doc> search(const Range<Kw>& query) const;
};

/******************************************************************************/
/* PiBas (Result-Hiding)                                                      */
/******************************************************************************/

template <IDbDoc_ DbDoc = Doc, class DbKw = Kw>
class PiBasResHidingBase : public ISdaUnderly<DbDoc, DbKw> {
    protected:
        Db<DbDoc, DbKw> db;
        ustring key1;
        ustring key2;
        IEncInd* encInd = nullptr;
        bool _isEmpty = false;

        ustring genQueryToken(const Range<DbKw>& query) const;

    public:
        PiBasResHidingBase() = default;
        PiBasResHidingBase(EncIndType encIndType);
        virtual ~PiBasResHidingBase();

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;

        std::vector<DbDoc> searchWithoutRemovingDels(const Range<DbKw>& query) const override;
        void clear() override;
        Db<DbDoc, DbKw> getDb() const override;
        bool isEmpty() const override;
        void setEncIndType(EncIndType encIndType) override;
};

template <IDbDoc_ DbDoc = Doc, class DbKw = Kw>
class PiBasResHiding : public PiBasResHidingBase<DbDoc, DbKw> {
    public:
        PiBasResHiding() = default;
        PiBasResHiding(EncIndType encIndType);

        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};

template <>
class PiBasResHiding<Doc, Kw> : public PiBasResHidingBase<Doc, Kw> {
    public:
        PiBasResHiding() = default;
        PiBasResHiding(EncIndType encIndType);

        std::vector<Doc> search(const Range<Kw>& query) const override;
};
