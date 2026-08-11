#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/sd_underly.h"
#include "schemes/interfaces/sse.h"
#include "schemes/pi_bas/pi_bas.h"

#include "utils/doc.h"
#include "utils/range.h"
#include "utils/sse_utils.h"
#include "utils/tdag.h"


// common code between `LogSrcI` and `LogSrcIStar`
template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
class LogSrcIBase : public ISdUnderly<Doc<>, Kw> {
public:
    using ISdUnderly<Doc<>, Kw>::ISdUnderly;

    virtual ~LogSrcIBase();

    //---------------------------------------------------------------------
    // `ISse`

    std::vector<Doc<>> search(
        const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
    ) const override;
    void clear() override;

    //----------------------------------------------------------------------
    // `ISdUnderly`

    void getDb(Db<Doc<>, Kw>& ret) const override;

protected:
    Underly<SrcIDb1Doc, Kw>* underly1 = new Underly<SrcIDb1Doc, Kw>(this->benchmark);
    Underly<Doc<IdAlias>, IdAlias>* underly2 = new Underly<Doc<IdAlias>, IdAlias>(this->benchmark);
    TdagNode<Kw>* tdag1 = nullptr;
    TdagNode<IdAlias>* tdag2 = nullptr;
    // this is only used to store the original db in `getDb()` so that it is encrypted
    // but easy to recover, instead of reconstructing the original db from `underly1`'s
    // and `underly2`'s indexes/dbs
    // do NOT search on this one!
    // also it's specifically PiBas since `LogSrcIStar` may use locality-aware `Underly`,
    // which doesn't store non-TDAG datasets correctly or efficiently (e.g. they might pad)
    PiBas<Doc<>, Kw>* origDbUnderly = new PiBas<Doc<>, Kw>(this->benchmark);
};
