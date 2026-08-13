#pragma once

#include <concepts>
#include <cstdint>
#include <vector>

#include "schemes/interfaces/sse_server.h"

#include "utils/db.h"
#include "utils/enc_ind.h"
#include "utils/types.h"
#include "utils/ustring.h"


template <class DbTuple = Tuple<>, class DbKw = Kw> requires IsValidDbParams<DbTuple, DbKw>
class NLogNServer : public ISseServer<DbTuple, DbKw> {
public:
    using ISseServer<DbTuple, DbKw>::ISseServer;

    ~NLogNServer();

    //----------------------------------------------------------------------
    // `ISseServer`

    void clear() override;

    //----------------------------------------------------------------------
    // other

    void setEncIndLvls(const std::vector<EncInd*>& encIndLvls);
    std::vector<EncInd*> getEncIndLvls() const;
    std::vector<EncIndVal> searchEncIndForBckt(
        int64_t lvl, uint64_t startPos, int64_t bcktSize, const ustring& label
    ) const;

    void setDbKwCountsDict(EncInd* dbKwCountsDict);
    bool getDbKwCount(uint64_t pos, const ustring& label, EncIndVal& ret) const;

protected:
    std::vector<EncInd*> encIndLvls;

private:
    EncInd* dbKwCountsDict = nullptr;

    //--------------------------------------------------------------------------
    // other

    int64_t calcAllEncIndLvlsBytes() const;
};
