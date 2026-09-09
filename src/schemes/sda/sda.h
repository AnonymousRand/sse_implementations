#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/i_dsse.h"
#include "schemes/interfaces/i_sd_underly.h"

#include "types/basic_types.h"
#include "types/db/db.h"
#include "types/doc.h"
#include "types/range.h"
#include "types/tuple.h"


// don't use template template param for `Underly` because they may have other deeper
// underlying schemes (e.g. `Sda<LogSrcI<PiBas>>`) and it gets complicated, so instead
// just specify all template params for `Underly` fully
template <IsSdUnderly Underly>
class Sda : public IDsse<Tuple<>> {
private:
    using DbDoc = typename IDsse<Tuple<>>::DbDoc;
    using DbKw = typename IDsse<Tuple<>>::DbKw;

public:
    using IDsse<Tuple<>>::IDsse;

    ~Sda();

    //--------------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<Tuple<>>& db) override;
    std::vector<Doc> search(
        const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
    ) const override;
    void clear() override;

    //--------------------------------------------------------------------------
    // `IDsse`

    void update(const Tuple<>& newTuple) override;

private:
    std::vector<Underly*> underlys;
    bigint firstEmptyInd;

    //--------------------------------------------------------------------------
    // helpers

    bigint calcFirstEmptyInd() const;
};
