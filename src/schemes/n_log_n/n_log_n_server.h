#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/sse_server.h"

#include "utils/enc_ind.h"
#include "utils/tuple.h"
#include "utils/types.h"
#include "utils/ustring.h"


template <IsDbTuple DbTuple = Tuple<>>
class NLogNServer : public ISseServer<DbTuple> {
public:
    using ISseServer<DbTuple>::ISseServer;

    ~NLogNServer();

    //----------------------------------------------------------------------
    // `ISseServer`

    void clear() override;

    //----------------------------------------------------------------------
    // helpers

    void setEncIndLvls(const std::vector<EncInd*>& encIndLvls);
    std::vector<EncInd*> getEncIndLvls() const;
    std::vector<EncIndVal> searchEncIndForBckt(
        bigint lvl, ubigint startPos, bigint bcktSize, const ustring& label
    ) const;

    void setDbKwCountsDict(EncInd* dbKwCountsDict);
    bool getDbKwCount(ubigint pos, const ustring& label, EncIndVal& ret) const;

protected:
    std::vector<EncInd*> encIndLvls;

private:
    EncInd* dbKwCountsDict = nullptr;

    //--------------------------------------------------------------------------
    // helpers

    bigint calcAllEncIndLvlsBytes() const;
};
