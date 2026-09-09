#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/i_sd_underly.h"
#include "schemes/interfaces/i_sse.h"

#include "types/basic_types.h"
#include "types/db/db.h"
#include "types/doc.h"
#include "types/range.h"
#include "types/tdag.h"
#include "types/tuple.h"


// common code between `LogSrcI` and `LogSrcIStar`
template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
class LogSrcIBase : public ISdUnderly<Tuple<>> {
public:
    virtual ~LogSrcIBase();

    //--------------------------------------------------------------------------
    // `ISse`

    std::vector<Doc> search(
        const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
    ) const override;
    void clear() override;

    //--------------------------------------------------------------------------
    // `ISdUnderly`

    void getDb(Db<Tuple<>>& ret) const override;

protected:
    Underly<SrcIDb1Tuple>* underly1 = new Underly<SrcIDb1Tuple>();
    Underly<Tuple<IdAlias>>* underly2 = new Underly<Tuple<IdAlias>>();
    TdagNode<Kw>* tdag1 = nullptr;
    TdagNode<IdAlias>* tdag2 = nullptr;
};
