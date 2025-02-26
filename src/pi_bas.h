#pragma once

#include "sse.h"
#include "util/openssl.h"

static const EVP_CIPHER* ENC_CIPHER = EVP_aes_256_cbc();
static const EVP_MD* HASH_FUNC = EVP_sha512();
static const int HASH_OUTPUT_LEN = 512;

////////////////////////////////////////////////////////////////////////////////
// PiBas
////////////////////////////////////////////////////////////////////////////////

template <IDbDocDeriv DbDoc = IdOp, class DbKw = Kw>
class PiBas : public ISse<DbDoc, DbKw> {
    private:
        Db<DbDoc, DbKw> db;
        ustring key;
        EncInd encInd;

    public:
        // API functions
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;

        Db<DbDoc, DbKw> getDb() const override;

        // non-API functions
        std::pair<ustring, ustring> genQueryToken(const Range<DbKw>& query) const;
        std::vector<DbDoc> searchInd(const std::pair<ustring, ustring>& queryToken) const;
        std::vector<DbDoc> searchIndBase(const std::pair<ustring, ustring>& queryToken) const;
};

////////////////////////////////////////////////////////////////////////////////
// PiBas (Result-Hiding)
///////////////////////////////////////////////////////////////////////////////

template <IDbDocDeriv DbDoc = IdOp, class DbKw = Kw>
class PiBasResHiding : public ISse<DbDoc, DbKw> {
    private:
        Db<DbDoc, DbKw> db;
        ustring key1;
        ustring key2;
        EncInd encInd;

    public:
        // API functions
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;

        Db<DbDoc, DbKw> getDb() const override;

        // non-API functions
        ustring genQueryToken(const Range<DbKw>& query) const;
        std::vector<DbDoc> searchInd(const ustring& queryToken) const;
        std::vector<DbDoc> searchIndBase(const ustring& queryToken) const;
};
