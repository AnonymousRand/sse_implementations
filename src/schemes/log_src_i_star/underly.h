#pragma once

#include <concepts>
#include <cstdint>
#include <vector>

#include "schemes/n_log_n/n_log_n.h" 

#include "utils/db.h"
#include "utils/range.h"
#include "utils/tuple.h"
#include "utils/types.h"


namespace log_src_i_star {


// this is specifcally designed to avoid using NLogN as a black box for Log-SRC-i*
// (the same way one may use PiBas) which blows up the storage unnecessarily,
// as observed in the TODS'18 paper (Section 7.1)
template <IsDbTuple DbTuple = Tuple<>>
class Underly : public NLogN<DbTuple> {
private:
    using DbKw = typename NLogN<DbTuple>::DbKw;

public:
    using NLogN<DbTuple>::NLogN;

    //----------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<DbTuple>& db) override;

private:
    int64_t leafCount;

    //----------------------------------------------------------------------
    // `IStaticPointSse`

    std::vector<DbTuple> searchBase(const Range<DbKw>& query) const override;

    //----------------------------------------------------------------------
    // helpers

    int64_t computeNumLvls() const override;
    int64_t computeBcktCountOnLvl(int64_t lvlNum) const override;
};


} // namespace `log_src_i_star`
