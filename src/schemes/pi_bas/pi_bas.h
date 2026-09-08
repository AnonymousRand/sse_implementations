#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/i_sd_underly.h"
#include "schemes/interfaces/i_static_point_sse.h"
#include "schemes/pi_bas/pi_bas_server.h"

#include "types/basic_types.h"
#include "types/db/db.h"
#include "types/range.h"
#include "types/tuple.h"
#include "types/ustring.h"


// note that we use the result-hiding variant of PiBas from figure 12 of NDSS'20
// (SDa paper) since SDa wants that
template <IsDbTuple DbTuple = Tuple<>>
class PiBas : public IStaticPointSse<DbTuple>, public ISdUnderly<DbTuple> {
private:
    using DbKw = typename IStaticPointSse<DbTuple>::DbKw;

public:
    ~PiBas();

    //--------------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<DbTuple>& db) override;
    void clear() override;

    //--------------------------------------------------------------------------
    // `ISdUnderly`

    void getDb(Db<DbTuple>& ret) const override;

private:
    PiBasServer<DbTuple>* server = new PiBasServer<DbTuple>();

    //--------------------------------------------------------------------------
    // `IStaticPointSse`

    std::vector<DbTuple> searchRaw(const Range<DbKw>& query) const override;

    //--------------------------------------------------------------------------
    // helpers

    ustring genQueryToken(const Range<DbKw>& query) const;

    /**
     * generate encrypted label to store in encrypted index, and also return
     * numerical position at which to place it in the index (pseudorandomly).
     */
    ubigint map(const ustring& queryToken, bigint dbKwListSize, ustring& retLabel) const;
};
