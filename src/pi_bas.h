#pragma once

#include "sse.h"
#include "util/openssl.h"

static const EVP_CIPHER* ENC_CIPHER = EVP_aes_256_cbc();
static const EVP_MD* HASH_FUNC = EVP_sha512();
static const int HASH_OUTPUT_LEN = 512;

////////////////////////////////////////////////////////////////////////////////
// PiBas
////////////////////////////////////////////////////////////////////////////////

template <IDbDocDeriv DbDoc, class DbKw>
class PiBasBase : public ISse<DbDoc, DbKw> {
    protected:
        Db<DbDoc, DbKw> db;
        ustring key;
        EncInd encInd;

        std::pair<ustring, ustring> genQueryToken(const Range<DbKw>& query) const;
        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const;

    public:
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;

        Db<DbDoc, DbKw> getDb() const override;
};

// this is literally just so I can partially specialize `searchInd()`'s template
template <IDbDocDeriv DbDoc = IdOp, class DbKw = Kw>
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

template <IDbDocDeriv DbDoc = IdOp, class DbKw = Kw>
class PiBasResHidingBase : public ISse<DbDoc, DbKw> {
    protected:
        Db<DbDoc, DbKw> db;
        ustring key1;
        ustring key2;
        EncInd encInd;

        ustring genQueryToken(const Range<DbKw>& query) const;
        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const;

    public:
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;

        Db<DbDoc, DbKw> getDb() const override;
};

template <IDbDocDeriv DbDoc = IdOp, class DbKw = Kw>
class PiBasResHiding : public PiBasResHidingBase<DbDoc, DbKw> {
    public:
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;
};

template <class DbKw>
class PiBasResHiding<IdOp, DbKw> : public PiBasResHidingBase<IdOp, DbKw> {
    public:
        std::vector<IdOp> search(const Range<DbKw>& query) const override;
};
