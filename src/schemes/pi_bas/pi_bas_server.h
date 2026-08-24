#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/sse_server.h"

#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_rand.h"
#include "utils/types/enc_ind/enc_ind_types.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


template <IsDbTuple DbTuple = Tuple<>>
class PiBasServer : public ISseServer<DbTuple> {
public:
    using ISseServer<DbTuple>::ISseServer;

    ~PiBasServer();

    //--------------------------------------------------------------------------
    // `ISseServer`

    void clear() override;

    //--------------------------------------------------------------------------
    // helpers

    void setEncInd(EncIndRand* encInd);
    EncIndRand* getEncInd() const;
    std::vector<EncIndVal> searchEncInd(const ustring& queryToken) const;

private:
    EncIndRand* encInd = nullptr;
};
