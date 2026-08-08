#pragma once

#include <cstdint>
#include <vector>

#include "benchmark.h" // since this was only forward declared in `sse_server.h`
#include "schemes/sse_server.h"
#include "utils/enc_ind.h"
#include "utils/ustring.h"


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
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
        EncInd* getEncInd() const;
        std::vector<EncIndVal> searchEncInd(const ustring& queryToken) const;

    private:
        EncInd* encInd = nullptr;
};
