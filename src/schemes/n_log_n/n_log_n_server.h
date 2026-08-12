#pragma once

#include <concepts>
#include <cstdint>
#include <vector>

#include "schemes/interfaces/sse_server.h"

#include "utils/db.h"
#include "utils/enc_ind.h"
#include "utils/sse_utils.h"
#include "utils/ustring.h"


template <class DbRecord = Record<>, class DbKw = Kw> requires IsValidDbParams<DbRecord, DbKw>
class NLogNServer : public ISseServer<DbRecord, DbKw> {
public:
    using ISseServer<DbRecord, DbKw>::ISseServer;

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
    // stuff to not share with Log-SRC-i*
    EncInd* dbKwCountsDict = nullptr;
};
