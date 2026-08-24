#pragma once

#include <concepts>
#include <memory>
#include <vector>

#include "schemes/n_log_n/n_log_n_base.h"
#include "schemes/n_log_n/n_log_n_server.h"

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


template <IsDbTuple DbTuple = Tuple<>>
class NLogN : public NLogNBase<DbTuple> {
private:
    using DbKw = typename NLogNBase<DbTuple>::DbKw;

public:
    using NLogNBase<DbTuple>::NLogNBase;

    //--------------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<DbTuple>& db) override;

protected:
    //--------------------------------------------------------------------------
    // `IStaticPointSse`

    std::vector<DbTuple> searchBase(const Range<DbKw>& query) const override;

    bigint calcLvlCount() const override;
    bigint calcBcktCountOnLvl(bigint lvl) const override;
    bigint calcBcktSizeOnLvl(bigint lvl) const override;

private:
    NLogNServer<DbTuple>* server = new NLogNServer<DbTuple>(this->benchmark);
    NLogNServer<DbTuple>* getServer() const override { return this->server; }
    void setServer(NLogNBaseServer<DbTuple>* server) override { this->server = server; }
};
