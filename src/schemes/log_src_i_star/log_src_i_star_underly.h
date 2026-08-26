#pragma once

#include <concepts>
#include <vector>

#include "schemes/log_src_i_star/log_src_i_star_underly_server.h"
#include "schemes/n_log_n/n_log_n_base.h" 

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


namespace log_src_i_star {


// this is specifcally designed to avoid using NLogN as a black box for Log-SRC-i*
// (the same way one may use PiBas) which blows up the storage unnecessarily,
// as observed in the TODS'18 paper (Section 7.1)
//
// also this doesn't technically have to be templated, but we need it so it can be passed
// as a template template parameter to `LogSrcIStar` like with other range schemes here
template <IsDbTuple DbTuple = Tuple<>>
class Underly : public NLogNBase<DbTuple> {
private:
    using DbKw = typename NLogNBase<DbTuple>::DbKw;

public:
    using NLogNBase<DbTuple>::NLogNBase;

    ~Underly();

    //--------------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<DbTuple>& db) override;
    void clear() override;

private:
    bigint leafCount;

    //--------------------------------------------------------------------------
    // `IStaticPointSse`

    std::vector<DbTuple> searchRaw(const Range<DbKw>& query) const override;

    //--------------------------------------------------------------------------
    // helpers

    bigint calcLvlCount() const override;
    bigint calcBcktCountOnLvl(bigint lvl) const override;
    bigint calcBcktSizeOnLvl(bigint lvl) const override;

private:
    UnderlyServer<DbTuple>* server = new UnderlyServer<DbTuple>(this->benchmark);
    UnderlyServer<DbTuple>* getServer() const override { return this->server; }
};


} // namespace `log_src_i_star`
