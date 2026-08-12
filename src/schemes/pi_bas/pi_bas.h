#pragma once

#include <concepts>
#include <cstdint>
#include <vector>

#include "schemes/interfaces/sd_underly.h"
#include "schemes/interfaces/static_point_sse.h"
#include "schemes/pi_bas/pi_bas_server.h"

#include "utils/db.h"
#include "utils/range.h"
#include "utils/types.h"
#include "utils/ustring.h"


// note that we use the result-hiding variant of PiBas from figure 12 of NDSS'20
// (SDa paper) since SDa wants that
template <class DbTuple = Tuple<>, class DbKw = Kw> requires IsValidDbParams<DbTuple, DbKw>
class PiBas : public IStaticPointSse<DbTuple, DbKw>, public ISdUnderly<DbTuple, DbKw> {
public:
    using IStaticPointSse<DbTuple, DbKw>::IStaticPointSse;

    ~PiBas();

    //----------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<DbTuple>& db) override;
    void clear() override;

    //----------------------------------------------------------------------
    // `ISdUnderly`

    void getDb(Db<DbTuple>& ret) const override;

private:
    PiBasServer<DbTuple, DbKw>* server = new PiBasServer<DbTuple, DbKw>(this->benchmark);

    //----------------------------------------------------------------------
    // `IStaticPointSse`

    std::vector<DbTuple> searchBase(const Range<DbKw>& query) const override;

    //----------------------------------------------------------------------
    // other

    ustring genQueryToken(const Range<DbKw>& query) const;

    /**
     * generate encrypted label to store in encrypted index, and also return
     * numerical position at which to place it in the index (pseudorandomly).
     */
    uint64_t map(const ustring& queryToken, int64_t dbKwListSize, ustring& retLabel) const;
};
