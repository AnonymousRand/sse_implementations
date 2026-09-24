#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/i_sse_server.h"

#include "utils/types/basic_types.h"
#include "utils/types/encr_ind/encr_ind_loc.h"
#include "utils/types/encr_ind/encr_ind_types.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


template <IsDbTuple DbTuple = Tuple<>>
class NLogNBaseServer : public ISseServer<DbTuple> {
public:
    virtual ~NLogNBaseServer();

    //--------------------------------------------------------------------------
    // `ISseServer`

    void clear() override;

    //--------------------------------------------------------------------------
    // interface

    void setEncrIndLvls(const std::vector<EncrIndLoc*>& encrIndLvls);
    const std::vector<EncrIndLoc*>& getEncrIndLvls() const;
    std::vector<EncrIndVal> searchEncrIndForBckt(
        bigint lvl, ubigint startPos, bigint bcktSize, const ustring& label
    ) const;

protected:
    std::vector<EncrIndLoc*> encrIndLvls;
};
