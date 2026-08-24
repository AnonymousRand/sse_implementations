#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/sse_server.h"

#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_loc.h"
#include "utils/types/enc_ind/enc_ind_rand.h"
#include "utils/types/enc_ind/enc_ind_types.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


template <IsDbTuple DbTuple = Tuple<>>
class NLogNServer : public ISseServer<DbTuple> {
public:
    using ISseServer<DbTuple>::ISseServer;

    ~NLogNServer();

    //--------------------------------------------------------------------------
    // `ISseServer`

    void clear() override;

    //--------------------------------------------------------------------------
    // helpers

    void setEncIndLvls(const std::vector<EncIndLoc*>& encIndLvls);
    std::vector<EncIndLoc*> getEncIndLvls() const;
    std::vector<EncIndVal> searchEncIndForBckt(
        bigint lvl, ubigint startPos, bigint bcktSize, const ustring& label
    ) const;

    void setDbKwCountsDict(EncIndRand* dbKwCountsDict);
    bool getDbKwCount(ubigint pos, const ustring& label, EncIndVal& ret) const;

protected:
    std::vector<EncIndLoc*> encIndLvls;

private:
    EncIndRand* dbKwCountsDict = nullptr;

    //--------------------------------------------------------------------------
    // helpers

    bigint calcAllEncIndLvlsBytes() const;
};
