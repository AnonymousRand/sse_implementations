#pragma once

#include <cstdint>
#include <vector>

#include "benchmark.h" // since this was only forward declared in `sse_server.h`

#include "schemes/sse_server.h"

#include "utils/enc_ind.h"
#include "utils/ustring.h"


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class NLogNServer : public ISseServer<DbDoc, DbKw> {
    public:
        using ISseServer<DbDoc, DbKw>::ISseServer;

        ~NLogNServer();

        //----------------------------------------------------------------------
        // `ISseServer`

        void clear() override;

        //----------------------------------------------------------------------
        // other

        void setEncIndLvls(const std::vector<EncInd*>& encIndLvls);
        std::vector<EncInd*> getEncIndLvls() const;
        std::vector<EncIndVal> findEncIndBckt(
            int64_t lvl, uint64_t startPos, int64_t bcktSize, const ustring& label
        ) const;

        void setDbKwCountsDict(EncInd* dbKwCountsDict);
        bool getDbKwCount(uint64_t pos, const ustring& label, EncIndVal& ret) const;

    protected:
        std::vector<EncInd*> encIndLvls;

    private:
        // stuff to not share with Log-SRC-i*
        EncInd* dbKwCountsDict = nullptr;
};
