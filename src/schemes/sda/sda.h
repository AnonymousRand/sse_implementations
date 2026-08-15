#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/dsse.h"
#include "schemes/interfaces/sd_underly.h"

#include "utils/db/db.h"
#include "utils/range.h"
#include "utils/tuple.h"
#include "utils/types.h"


// don't use template template param for `Underly` because they may have other deeper
// underlying schemes (e.g. `Sda<LogSrcI<PiBas>>`) and it gets complicated, so instead
// just specify all template params for `Underly` fully
template <IsSdUnderly Underly>
class Sda : public IDsse<Tuple<>> {
private:
    using DbKw = typename IDsse<Tuple<>>::DbKw;

public:
    using IDsse<Tuple<>>::IDsse;

    ~Sda();

    //----------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<Tuple<>>& db) override;
    std::vector<Tuple<>> search(
        const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
    ) const override;
    void clear() override;

    //----------------------------------------------------------------------
    // `IDsse`

    void update(const Tuple<>& newTuple) override;

private:
    std::vector<Underly*> underlys;
    bigint firstEmptyInd;
};
