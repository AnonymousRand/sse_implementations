#pragma once

#include <concepts>
#include <vector>

#include "schemes/log_src_i_star/log_src_i_star_underly_server.h"
#include "schemes/n_log_n/n_log_n_base.h" 

#include "types/basic_types.h"
#include "types/db/db.h"
#include "types/range.h"
#include "types/tuple.h"


namespace log_src_i_star {


// this is specifically designed to avoid using NLogN as a black box for Log-SRC-i*
// (the same way one may use PiBas) which blows up the storage unnecessarily,
// as observed in the TODS'18 paper (Section 7.1)
//
// also this doesn't technically have to be templated, but we need it so it can be passed
// as a template template parameter to `LogSrcIStar` like with other range schemes here
template <IsDbTuple DbTuple = Tuple<>>
class Underly : public NLogNBase<DbTuple> {
private:
    using DbDoc = typename NLogNBase<DbTuple>::DbDoc;
    using DbKw = typename NLogNBase<DbTuple>::DbKw;

public:
    ~Underly();

    //--------------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<DbTuple>& db) override;
    void clear() override;

private:
    bigint leafCount;

    //--------------------------------------------------------------------------
    // `NLogNBase`

    UnderlyServer<DbTuple>* server = new UnderlyServer<DbTuple>();
    UnderlyServer<DbTuple>* getServer() const override { return this->server; }

    //--------------------------------------------------------------------------
    // `IStaticPointSse`

    std::vector<DbDoc> searchRaw(const Range<DbKw>& query) const override;

    //--------------------------------------------------------------------------
    // helpers

    bigint calcLvlCount() const override;
    bigint calcBcktCountOnLvl(bigint lvl) const override;
    bigint calcBcktSizeOnLvl(bigint lvl) const override;
};


} // namespace `log_src_i_star`
