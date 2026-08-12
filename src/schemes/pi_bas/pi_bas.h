#pragma once

#include <concepts>
#include <cstdint>
#include <vector>

#include "schemes/interfaces/sd_underly.h"
#include "schemes/interfaces/static_point_sse.h"
#include "schemes/pi_bas/pi_bas_server.h"

#include "utils/db.h"
#include "utils/range.h"
#include "utils/sse_utils.h"
#include "utils/ustring.h"


// note that we use the result-hiding variant of PiBas from figure 12 of NDSS'20
// (SDa paper) since SDa wants that
template <class DbRecord = Record<>, class DbKw = Kw> requires IsValidDbParams<DbRecord, DbKw>
class PiBas : public IStaticPointSse<DbRecord, DbKw>, public ISdUnderly<DbRecord, DbKw> {
public:
    using IStaticPointSse<DbRecord, DbKw>::IStaticPointSse;

    ~PiBas();

    //----------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<DbRecord, DbKw>& db) override;
    void clear() override;

    //----------------------------------------------------------------------
    // `ISdUnderly`

    void getDb(Db<DbRecord, DbKw>& ret) const override;

private:
    PiBasServer<DbRecord, DbKw>* server = new PiBasServer<DbRecord, DbKw>(this->benchmark);

    //----------------------------------------------------------------------
    // `IStaticPointSse`

    std::vector<DbRecord> searchBase(const Range<DbKw>& query) const override;

    //----------------------------------------------------------------------
    // other

    ustring genQueryToken(const Range<DbKw>& query) const;

    /**
     * generate encrypted label to store in encrypted index, and also return
     * numerical position at which to place it in the index (pseudorandomly).
     */
    uint64_t map(const ustring& queryToken, int64_t dbKwListSize, ustring& retLabel) const;
};
