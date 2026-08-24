#pragma once

#include <concepts>

#include "schemes/n_log_n/n_log_n_base_server.h"

#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_rand.h"
#include "utils/types/enc_ind/enc_ind_types.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


template <IsDbTuple DbTuple = Tuple<>>
class NLogNServer : public NLogNBaseServer<DbTuple> {
public:
    using NLogNBaseServer<DbTuple>::NLogNBaseServer;

    //--------------------------------------------------------------------------
    // `ISseServer`

    void clear() override;

    //--------------------------------------------------------------------------
    // interface

    void setDbKwCountsDict(EncIndRand* dbKwCountsDict);
    bool getDbKwCount(ubigint pos, const ustring& label, EncIndVal& ret) const;

private:
    EncIndRand* dbKwCountsDict = nullptr;
};
