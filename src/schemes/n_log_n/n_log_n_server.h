#pragma once

#include <concepts>

#include "schemes/n_log_n/n_log_n_base_server.h"

#include "utils/types/basic_types.h"
#include "utils/types/encr_ind/encr_ind_rand.h"
#include "utils/types/encr_ind/encr_ind_types.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


template <IsDbTuple DbTuple = Tuple<>>
class NLogNServer : public NLogNBaseServer<DbTuple> {
public:
    //--------------------------------------------------------------------------
    // `ISseServer`

    void clear() override;

    //--------------------------------------------------------------------------
    // interface

    void setDbKwCountsDict(EncrIndRand* dbKwCountsDict);
    bool getDbKwCount(ubigint pos, const ustring& label, EncrIndVal& ret) const;

private:
    EncrIndRand* dbKwCountsDict = nullptr;
};
