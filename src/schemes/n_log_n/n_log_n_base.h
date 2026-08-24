#pragma once

#include <concepts>
#include <utility>
#include <vector>

#include "schemes/interfaces/sd_underly.h"
#include "schemes/interfaces/static_point_sse.h"
#include "schemes/n_log_n/n_log_n_base_server.h"

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


template <IsDbTuple DbTuple = Tuple<>>
class NLogNBase : public IStaticPointSse<DbTuple>, public ISdUnderly<DbTuple> {
protected:
    using DbKw = typename IStaticPointSse<DbTuple>::DbKw;

public:
    using IStaticPointSse<DbTuple>::IStaticPointSse;

    virtual ~NLogNBase();

    //--------------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<DbTuple>& db) override;
    void clear() override;

    //--------------------------------------------------------------------------
    // `ISdUnderly`

    void getDb(Db<DbTuple>& ret) const override;

protected:
    virtual NLogNBaseServer<DbTuple>* getServer() const = 0;
    bigint lvlCount = 0;

    //--------------------------------------------------------------------------
    // helpers

    ustring genQueryToken(const Range<DbKw>& query) const;

    /**
     * generate encrypted label to store in encrypted index, and also return numerical
     * position only at which to place it in the index with no modulo for bucket count.
     */
    ubigint mapNoMod(const ustring& queryToken, ustring& retLabel) const;

    /**
     * generate encrypted label to store in encrypted index, and also return numerical level
     * and position at which to place it in the index. (position is a bucket count, not
     * entry count, so this is the raw position mod the bucket count on that level.)
     *
     * preconditions:
     *     - `dbKwListSize` is a power of 2.
     */
    std::pair<ubigint, ubigint> map(
        const ustring& queryToken, bigint dbKwPaddedCount, ustring& retLabel
    ) const;

    virtual bigint calcLvlCount() const = 0;
    virtual bigint calcBcktCountOnLvl(bigint lvl) const = 0;
    virtual bigint calcBcktSizeOnLvl(bigint lvl) const = 0;
};
