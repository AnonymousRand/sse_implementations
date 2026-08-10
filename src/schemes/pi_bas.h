#pragma once

#include <concepts>
#include <cstdint>
#include <vector>

#include "schemes/pi_bas_server.h"
#include "schemes/interfaces/sd_underly.h"
#include "schemes/interfaces/static_point_sse.h"

#include "utils/doc.h"
#include "utils/range.h"
#include "utils/sse_utils.h"
#include "utils/ustring.h"


// note that we use the result-hiding variant of PiBas from figure 12 of NDSS'20 (SDa paper) since SDa wants that
template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class PiBas : public IStaticPointSse<DbDoc, DbKw>, public ISdUnderly<DbDoc, DbKw> {
public:
    using IStaticPointSse<DbDoc, DbKw>::IStaticPointSse;

    ~PiBas();

    //----------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
    void clear() override;

    //----------------------------------------------------------------------
    // `ISdUnderly`

    void getDb(Db<DbDoc, DbKw>& ret) const override;

private:
    PiBasServer<DbDoc, DbKw>* server = new PiBasServer<DbDoc, DbKw>(this->benchmark);

    //----------------------------------------------------------------------
    // `IStaticPointSse`

    std::vector<DbDoc> searchBase(const Range<DbKw>& query) const override;

    //----------------------------------------------------------------------
    // other

    ustring genQueryToken(const Range<DbKw>& query) const;

    /**
     * generate encrypted label to store in encrypted index, and also return
     * numerical position at which to place it in the index (pseudorandomly).
     */
    uint64_t map(const ustring& queryToken, int64_t dbKwListSize, ustring& retLabel) const;
};
