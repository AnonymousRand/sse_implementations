#pragma once

#include "schemes/sse_server.h"
#include "utils/benchmark.h" // since this was only forward declared in `sse_server.h`
#include "utils/enc_ind.h"
#include "utils/utils.h"


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams <DbDoc, DbKw>
class PiBasServer : public ISseServer<DbDoc, DbKw> {
    public:
        using ISseServer<DbDoc, DbKw>::ISseServer;

        ~PiBasServer();

        //----------------------------------------------------------------------
        // `ISseServer`

        void clear() override;

        //----------------------------------------------------------------------
        // other

        void setEncInd(EncInd* encInd);
        std::vector<EncIndVal> search(const ustring& queryToken) const;

        bool getEncIndVal(ulong pos, EncIndVal& ret) const;

    private:
        EncInd* encInd = nullptr;
};
