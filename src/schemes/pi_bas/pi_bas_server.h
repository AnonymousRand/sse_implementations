#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/sse_server.h"

#include "utils/db.h"
#include "utils/enc_ind.h"
#include "utils/sse_utils.h"
#include "utils/ustring.h"


template <class DbTuple = Tuple<>, class DbKw = Kw> requires IsValidDbParams<DbTuple, DbKw>
class PiBasServer : public ISseServer<DbTuple, DbKw> {
public:
    using ISseServer<DbTuple, DbKw>::ISseServer;

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
