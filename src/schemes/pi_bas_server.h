#pragma once

#include <vector>

#include "utils/benchmark.h"
#include "utils/enc_ind.h"
#include "utils/utils.h"


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams <DbDoc, DbKw>
class PiBasServer {
    public:
        // TODO: maybe move just benchmarking stuff to a server interface? along with a clear()?
        // this should be the client/controller's benchmarking struct
        Benchmark& benchmark;

        PiBasServer(Benchmark& benchmark);
        ~PiBasServer();

        void clear();
        void setEncInd(EncInd* encInd);
        std::vector<EncIndVal> search(const ustring& queryToken) const;

        bool getEncIndVal(ulong pos, EncIndVal& ret) const;

    private:
        EncInd* encInd;
};
