#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/sd_underly.h"
#include "schemes/interfaces/sse.h"

#include "utils/db.h"
#include "utils/range.h"
#include "utils/tdag.h"
#include "utils/types.h"


// common code between `LogSrcI` and `LogSrcIStar`
template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>, Kw>>
class LogSrcIBase : public ISdUnderly<Tuple<>, Kw> {
public:
    using ISdUnderly<Tuple<>, Kw>::ISdUnderly;

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
    Underly<SrcIDb1Tuple, Kw>* underly1 = new Underly<SrcIDb1Tuple, Kw>(this->benchmark);
    Underly<Tuple<IdAlias>, IdAlias>* underly2 = new Underly<Tuple<IdAlias>, IdAlias>(this->benchmark);
    TdagNode<Kw>* tdag1 = nullptr;
    TdagNode<IdAlias>* tdag2 = nullptr;
};
