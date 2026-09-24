#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/i_sse_server.h"

#include "utils/types/basic_types.h"
#include "utils/types/encr_ind/encr_ind_rand.h"
#include "utils/types/encr_ind/encr_ind_types.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


template <IsDbTuple DbTuple = Tuple<>>
class PiBasServer : public ISseServer<DbTuple> {
public:
    ~PiBasServer();

    //--------------------------------------------------------------------------
    // `ISseServer`

    void clear() override;

    //--------------------------------------------------------------------------
    // interface

    void setEncrInd(EncrIndRand* encrInd);
    EncrIndRand* getEncrInd() const;
    std::vector<EncrIndVal> searchEncrInd(const ustring& queryToken) const;

private:
    EncrIndRand* encrInd = nullptr;
};
