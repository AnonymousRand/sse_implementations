#pragma once

#include <cstdint>
#include <vector>

#include "schemes/sse_server.h"
#include "utils/benchmark.h" // since this was only forward declared in `sse_server.h`
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

        void addEncIndLvl(EncInd* encIndLvl);
        void writeToEncInd(int64_t lvl, uint64_t pos, const EncIndEntry& entry);
        std::vector<EncIndVal> findEncIndBucket(
            int64_t lvl, uint64_t startPos, int64_t bucketSize, const ustring& label
        ) const;
        bool getEncIndVal(int64_t lvl, uint64_t pos, EncIndVal& ret) const;

        void initDbKwCountsDict(int64_t size);
        void writeToDbKwCountsDict(uint64_t pos, const EncIndEntry& entry);
        bool getDbKwCount(uint64_t pos, const ustring& label, EncIndVal& ret);

    protected:
        std::vector<EncInd*> encIndLvls;

    private:
        // stuff to not share with Log-SRC-i*
        EncInd* dbKwCountsDict = new EncInd();
};
