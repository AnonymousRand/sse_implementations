#pragma once

#include <concepts>
#include <vector>

#include "schemes/n_log_n/n_log_n_base.h"
#include "schemes/n_log_n/n_log_n_server.h"

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/encr_ind/encr_ind_rand.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


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
    EncrIndRand* dbKwCountsDictTmp = nullptr;

    //--------------------------------------------------------------------------
    // `IStaticPointSse`

    std::vector<DbDoc> searchRaw(const Range<DbKw>& query) const override;

    //--------------------------------------------------------------------------
    // `NLogNBase`

    NLogNServer<DbTuple>* nLogNServer = new NLogNServer<DbTuple>();
    NLogNServer<DbTuple>* getServer() const override { return this->nLogNServer; }

    void initSetupState(SseOper setupOper) override;
    void setupDbKwList(
        Db<DbTuple>&& dbKwList, const Range<DbKw>& dbKwRange, SseOper setupOper
    ) override;
    void moveSetupStateToServer(SseOper setupOper) override;

    bigint calcLvlCount() const override;
    bigint calcBcktCountOnLvl(bigint lvl) const override;
    bigint calcBcktSizeOnLvl(bigint lvl) const override;
};
