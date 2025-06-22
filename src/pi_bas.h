#pragma once

#include "sse.h"
#include "util/enc_ind.h"

////////////////////////////////////////////////////////////////////////////////
// PiBas
////////////////////////////////////////////////////////////////////////////////

template <IDbDoc_ DbDoc, class DbKw>
class PiBasBase : public ISdaUnderly<DbDoc, DbKw> {
    protected:
        Db<DbDoc, DbKw> db;
        ustring key;
        // TODO templatize this
        RamEncInd encInd;
        bool _isEmpty = false;

        std::pair<ustring, ustring> genQueryToken(const Range<DbKw>& query) const;

    public:
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;

        std::vector<DbDoc> searchWithoutHandlingDels(const Range<DbKw>& query) const override;
        Db<DbDoc, DbKw> getDb() const override;
        bool isEmpty() const override;
};

// this is literally just so I can partially specialize `search()`'s template
template <IDbDoc_ DbDoc = IdOp, class DbKw = Kw>
class PiBas : public PiBasBase<DbDoc, DbKw> {
    public:
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};

template <class DbKw>
class PiBas<IdOp, DbKw> : public PiBasBase<IdOp, DbKw> {
    public:
        std::vector<IdOp> search(const Range<DbKw>& query) const;
};

////////////////////////////////////////////////////////////////////////////////
// PiBas (Result-Hiding)
///////////////////////////////////////////////////////////////////////////////

template <IDbDoc_ DbDoc = IdOp, class DbKw = Kw>
class PiBasResHidingBase : public ISdaUnderly<DbDoc, DbKw> {
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

template <IDbDoc_ DbDoc = IdOp, class DbKw = Kw>
class PiBasResHiding : public PiBasResHidingBase<DbDoc, DbKw> {
    public:
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};

template <class DbKw>
class PiBasResHiding<IdOp, DbKw> : public PiBasResHidingBase<IdOp, DbKw> {
    public:
        std::vector<IdOp> search(const Range<DbKw>& query) const override;
};
