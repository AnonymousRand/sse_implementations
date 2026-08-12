#pragma once

#include <concepts>
#include <cstdint>
#include <vector>

#include "schemes/interfaces/dsse.h"
#include "schemes/interfaces/sd_underly.h"

#include "utils/db.h"
#include "utils/range.h"
#include "utils/sse_utils.h"


// don't use template template param for `Underly` because they may have other deeper
// underlying schemes (e.g. `Sda<LogSrcI<PiBas>>`) and it gets complicated, so instead
// just specify all template params for `Underly` fully
template <IsSdUnderly Underly>
class Sda : public IDsse<Record<>, Kw> {
public:
    using IDsse<Record<>, Kw>::IDsse;

    ~Sda();

    //----------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<Record<>, Kw>& db) override;
    std::vector<Record<>> search(
        const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
    ) const override;
    void clear() override;

    //----------------------------------------------------------------------
    // `IDsse`

    void update(const DbRecord<Record<>, Kw>& newDbRecord) override;

private:
    std::vector<Underly*> underlys;
    int64_t firstEmptyInd;
};
