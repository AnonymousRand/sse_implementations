#pragma once

#include "sse.h"

////////////////////////////////////////////////////////////////////////////////
// PiBas
////////////////////////////////////////////////////////////////////////////////

template <IDbDocDeriv DbDoc = IdOp, class DbKw = Kw>
class PiBas : public ISse<DbDoc, DbKw> {
    private:
        ustring key;
        EncInd encInd;

    public:
        // API functions
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;

        // non-API functions
        QueryToken genQueryToken(const Range<DbKw>& query) const;
        std::vector<DbDoc> searchInd(const QueryToken& queryToken) const;
        std::vector<DbDoc> searchIndBase(const QueryToken& queryToken) const;
};

////////////////////////////////////////////////////////////////////////////////
// PiBas (Result-Hiding)
///////////////////////////////////////////////////////////////////////////////

// >todo have to make result-hiding pibas variant for dynamic; also track if index is emtpy (ie db passed to setup is empty? depending on how clear is done)
template <IDbDocDeriv DbDoc = IdOp, class DbKw = Kw>
class PiBasResHiding : public ISse<DbDoc, DbKw> {
    private:
        ustring key;
        EncInd encInd;

    public:
        // API functions
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        std::vector<DbDoc> search(const Range<DbKw>& query) const override;

        // non-API functions
        QueryToken genQueryToken(const Range<DbKw>& query) const;
        std::vector<DbDoc> searchInd(const QueryToken& queryToken) const;
        std::vector<DbDoc> searchIndBase(const QueryToken& queryToken) const;
};
