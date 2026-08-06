#pragma once

#include "utils/benchmark.h"
#include "utils/enc_ind.h"
#include "utils/utils.h"


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class NLogNServer {
    public:
        // this should be the client/controller's benchmarking struct
        Benchmark& benchmark;

        NLogNServer(Benchmark& benchmark);
        ~NLogNServer();

        void clear();

        void addEncIndLvl(EncInd* encIndLvl);
        void writeToEncInd(long lvl, ulong pos, const EncIndEntry& entry);
        std::vector<EncIndVal> findEncIndBucket(long lvl, ulong startPos, long bucketSize, const ustring& label) const;
        bool getEncIndVal(long lvl, ulong pos, EncIndVal& ret) const;

        void initDbKwCountsDict(long size);
        void writeToDbKwCountsDict(ulong pos, const EncIndEntry& entry);
        bool getDbKwCount(ulong pos, const ustring& label, EncIndVal& ret);

    protected:
        std::vector<EncInd*> encIndLvls;

    private:
        // stuff to not share with Log-SRC-i*
        EncInd* dbKwCountsDict = new EncInd();
};
