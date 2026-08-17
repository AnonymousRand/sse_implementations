#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/sd_underly.h"
#include "schemes/interfaces/sse.h"

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tdag.h"
#include "utils/types/tuple.h"


// common code between `LogSrcI` and `LogSrcIStar`
template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
class LogSrcIBase : public ISdUnderly<Tuple<>> {
protected:
    using DbKw = typename ISdUnderly<Tuple<>>::DbKw;

public:
    using ISdUnderly<Tuple<>>::ISdUnderly;

    virtual ~LogSrcIBase();

    //---------------------------------------------------------------------
    // `ISse`

    std::vector<Tuple<>> search(
        const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
    ) const override;
    void clear() override;

    //----------------------------------------------------------------------
    // `ISdUnderly`

    void getDb(Db<Tuple<>>& ret) const override;

protected:
    Underly<SrcIDb1Tuple>* underly1 = new Underly<SrcIDb1Tuple>(this->benchmark);
    Underly<Tuple<IdAlias>>* underly2 = new Underly<Tuple<IdAlias>>(this->benchmark);
    TdagNode<Kw>* tdag1 = nullptr;
    TdagNode<IdAlias>* tdag2 = nullptr;
};
