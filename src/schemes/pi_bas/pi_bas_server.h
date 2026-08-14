#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/sse_server.h"

#include "utils/enc_ind.h"
#include "utils/tuple.h"
#include "utils/types.h"
#include "utils/ustring.h"


template <IsDbTuple DbTuple = Tuple<>>
class PiBasServer : public ISseServer<DbTuple> {
public:
    using ISseServer<DbTuple>::ISseServer;

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
