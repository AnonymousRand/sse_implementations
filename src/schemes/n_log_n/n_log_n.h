#pragma once

#include <concepts>
#include <vector>

#include "schemes/n_log_n/n_log_n_base.h"
#include "schemes/n_log_n/n_log_n_server.h"

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/enc_ind/enc_ind_rand.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


template <IsDbTuple DbTuple = Tuple<>>
class NLogN : public NLogNBase<DbTuple> {
private:
    using DbKw = typename NLogNBase<DbTuple>::DbKw;

public:
    using NLogNBase<DbTuple>::NLogNBase;

    ~NLogN();

    //--------------------------------------------------------------------------
    // `ISse`

    void clear() override;

protected:
    EncIndRand* dbKwCountsDictTmp = nullptr;

    //--------------------------------------------------------------------------
    // `IStaticPointSse`

    std::vector<DbTuple> searchBase(const Range<DbKw>& query) const override;

    //--------------------------------------------------------------------------
    // helpers

    void initSetupState() override;
    void setupDbKwList(Db<DbTuple>&& dbKwList, const Range<DbKw>& dbKwRange) override;
    void moveSetupStateToServer() override;

    bigint calcLvlCount() const override;
    bigint calcBcktCountOnLvl(bigint lvl) const override;
    bigint calcBcktSizeOnLvl(bigint lvl) const override;

private:
    NLogNServer<DbTuple>* server = new NLogNServer<DbTuple>(this->benchmark);
    NLogNServer<DbTuple>* getServer() const override { return this->server; }
};
