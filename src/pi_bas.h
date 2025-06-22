#pragma once

#include "sse.h"
#include "util/enc_ind.h"

////////////////////////////////////////////////////////////////////////////////
// PiBas
////////////////////////////////////////////////////////////////////////////////

template <IEncInd_ EncInd, IDbDoc_ DbDoc, class DbKw>
class PiBasBase : public ISdaUnderly<EncInd, DbDoc, DbKw> {
    protected:
        Db<DbDoc, DbKw> db;
        ustring key;
        EncInd encInd;
        bool _isEmpty = false;

        std::pair<ustring, ustring> genQueryToken(const Range<DbKw>& query) const;

    public:
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;

        std::vector<DbDoc> searchWithoutHandlingDels(const Range<DbKw>& query) const override;
        Db<DbDoc, DbKw> getDb() const override;
        bool isEmpty() const override;
};

// this is literally just so I can partially specialize `search()`'s template for when `DbDoc` is `IdOp`
template <IEncInd_ EncInd, IDbDoc_ DbDoc = IdOp, class DbKw = Kw>
class PiBas : public PiBasBase<EncInd, DbDoc, DbKw> {
    public:
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};

template <IEncInd_ EncInd, class DbKw>
class PiBas<EncInd, IdOp, DbKw> : public PiBasBase<EncInd, IdOp, DbKw> {
    public:
        std::vector<IdOp> search(const Range<DbKw>& query) const;
};

////////////////////////////////////////////////////////////////////////////////
// PiBas (Result-Hiding)
///////////////////////////////////////////////////////////////////////////////

template <IEncInd_ EncInd, IDbDoc_ DbDoc = IdOp, class DbKw = Kw>
class PiBasResHidingBase : public ISdaUnderly<EncInd, DbDoc, DbKw> {
    protected:
        Db<DbDoc, DbKw> db;
        ustring key1;
        ustring key2;
        EncInd encInd;
        bool _isEmpty = false;

        ustring genQueryToken(const Range<DbKw>& query) const;

    public:
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;

        std::vector<DbDoc> searchWithoutHandlingDels(const Range<DbKw>& query) const override;
        Db<DbDoc, DbKw> getDb() const override;
        bool isEmpty() const override;
};

template <IEncInd_ EncInd, IDbDoc_ DbDoc = IdOp, class DbKw = Kw>
class PiBasResHiding : public PiBasResHidingBase<EncInd, DbDoc, DbKw> {
    public:
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};

template <IEncInd_ EncInd, class DbKw>
class PiBasResHiding<EncInd, IdOp, DbKw> : public PiBasResHidingBase<EncInd, IdOp, DbKw> {
    public:
        std::vector<IdOp> search(const Range<DbKw>& query) const override;
};
