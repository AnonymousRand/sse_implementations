#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/sse_server.h"

#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_loc.h"
#include "utils/types/enc_ind/enc_ind_types.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


template <IsDbTuple DbTuple = Tuple<>>
class NLogNBaseServer : public ISseServer<DbTuple> {
public:
    using ISseServer<DbTuple>::ISseServer;

    virtual ~NLogNBaseServer();

    //--------------------------------------------------------------------------
    // `ISseServer`

    void clear() override;

    //--------------------------------------------------------------------------
    // interface

    void setEncIndLvls(const std::vector<EncIndLoc*>& encIndLvls);
    std::vector<EncIndLoc*> getEncIndLvls() const;
    std::vector<EncIndVal> searchEncIndForBckt(
        bigint lvl, ubigint startPos, bigint bcktSize, const ustring& label
    ) const;

protected:
    std::vector<EncIndLoc*> encIndLvls;
};
