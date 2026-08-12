#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/sse_server.h"

#include "utils/db.h"
#include "utils/enc_ind.h"
#include "utils/sse_utils.h"
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
