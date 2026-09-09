#pragma once

#include <concepts>
#include <vector>

#include "schemes/n_log_n/n_log_n_base.h"
#include "schemes/n_log_n/n_log_n_server.h"

#include "types/basic_types.h"
#include "types/db/db.h"
#include "types/enc_ind/enc_ind_rand.h"
#include "types/range.h"
#include "types/tuple.h"


template <IsDbTuple DbTuple = Tuple<>>
class NLogN : public NLogNBase<DbTuple> {
private:
    using DbDoc = typename NLogNBase<DbTuple>::DbDoc;
    using DbKw = typename NLogNBase<DbTuple>::DbKw;

public:
    ~NLogN();

    //--------------------------------------------------------------------------
    // `ISse`

    void clear() override;

private:
    EncIndRand* dbKwCountsDictTmp = nullptr;

    //--------------------------------------------------------------------------
    // `NLogNBase`

    NLogNServer<DbTuple>* server = new NLogNServer<DbTuple>();
    NLogNServer<DbTuple>* getServer() const override { return this->server; }

    //--------------------------------------------------------------------------
    // `IStaticPointSse`

    std::vector<DbDoc> searchRaw(const Range<DbKw>& query) const override;

    //--------------------------------------------------------------------------
    // helpers

    void initSetupState() override;
    void setupDbKwList(Db<DbTuple>&& dbKwList, const Range<DbKw>& dbKwRange) override;
    void moveSetupStateToServer() override;

    bigint calcLvlCount() const override;
    bigint calcBcktCountOnLvl(bigint lvl) const override;
    bigint calcBcktSizeOnLvl(bigint lvl) const override;
};
