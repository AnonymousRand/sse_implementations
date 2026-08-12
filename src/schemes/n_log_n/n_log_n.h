#pragma once

#include <concepts>
#include <cstdint>
#include <utility>
#include <vector>

#include "schemes/interfaces/sd_underly.h"
#include "schemes/interfaces/static_point_sse.h"
#include "schemes/n_log_n/n_log_n_server.h"

#include "utils/db.h"
#include "utils/range.h"
#include "utils/sse_utils.h"
#include "utils/ustring.h"


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class NLogN : public IStaticPointSse<DbDoc, DbKw>, public ISdUnderly<DbDoc, DbKw> {
public:
    using IStaticPointSse<DbDoc, DbKw>::IStaticPointSse;

    ~NLogN();

    //----------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
    void clear() override;

    //----------------------------------------------------------------------
    // `ISdUnderly`

    void getDb(Db<DbDoc, DbKw>& ret) const override;

protected:
    NLogNServer<DbDoc, DbKw>* server = new NLogNServer<DbDoc, DbKw>(this->benchmark);
    int64_t numLvls;

    //----------------------------------------------------------------------
    // `IStaticPointSse`

    std::vector<DbDoc> searchBase(const Range<DbKw>& query) const override;

    //----------------------------------------------------------------------
    // other

    ustring genQueryToken(const Range<DbKw>& query) const;

    /**
     * generate encrypted label to store in encrypted index, and also return numerical
     * position only at which to place it in the index with no modulo for bucket count.
     */
    uint64_t mapNoMod(const ustring& queryToken, ustring& retLabel) const;

    /**
     * generate encrypted label to store in encrypted index, and also return
     * numerical level and position at which to place it in the index.
     * (position is a bucket count, not entry count, so this is the raw position
     * mod the bucket count on that level.)
     *
     * preconditions:
     *     - `dbKwListSize` is a power of 2.
     */
    std::pair<uint64_t, uint64_t> map(
        const ustring& queryToken, int64_t dbKwPaddedCount, ustring& retLabel
    ) const;

    virtual int64_t computeNumLvls() const;
    virtual int64_t computeBcktCountOnLvl(int64_t lvlNum) const;
    virtual int64_t computeBcktSizeOnLvl(int64_t lvlNum) const;
};
