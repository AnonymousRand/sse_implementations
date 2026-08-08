#pragma once

#include "schemes/pi_bas.h"
#include "schemes/interfaces/sda_underly.h"
#include "schemes/interfaces/sse.h"

#include "utils/tdag.h"


//------------------------------------------------------------------------------
// `LogSrcIBase`
//------------------------------------------------------------------------------


// common code between `LogSrcI` and `LogSrcIStar`
template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
class LogSrcIBase : public ISdaUnderly<Doc<>, Kw> {
public:
    using ISdaUnderly<Doc<>, Kw>::ISdaUnderly;

    virtual ~LogSrcIBase();

    //---------------------------------------------------------------------
    // `ISse`

    std::vector<Doc<>> search(
        const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
    ) const override;
    void clear() override;

    //----------------------------------------------------------------------
    // `ISdaUnderly`

    void getDb(Db<Doc<>, Kw>& ret) const override;

protected:
    Underly<SrcIDb1Doc, Kw>* underly1 = new Underly<SrcIDb1Doc, Kw>(this->benchmark);
    Underly<Doc<IdAlias>, IdAlias>* underly2 = new Underly<Doc<IdAlias>, IdAlias>(this->benchmark);
    // this is only used to store the original db in `getDb()` so that it is encrypted but easy to recover
    // instead of reconstructing the original db from `underly1`'s and `underly2`'s indexes/dbs
    // do NOT search on this one!
    // also it's specifically PiBas since `LogSrcIStar` may use locality-aware `Underly`,
    // which doesn't store non-TDAG-structured datasets correctly (e.g. it might have padding)
    PiBas<Doc<>, Kw>* origDbUnderly = new PiBas<Doc<>, Kw>(this->benchmark);
    TdagNode<Kw>* tdag1 = nullptr;
    TdagNode<IdAlias>* tdag2 = nullptr;
};


//------------------------------------------------------------------------------
// `LogSrcI`
//------------------------------------------------------------------------------


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
class LogSrcI : public LogSrcIBase<Underly> {
public:
    using LogSrcIBase<Underly>::LogSrcIBase;

    //----------------------------------------------------------------------
    // `ISse`

    /**
     * preconditions:
     *     - entries in `db` must have size 1 `Kw` ranges, i.e. a singular `Kw` value.
     *     - entries in `db` cannot have keyword equal to `DUMMY`.
     */
    void setup(int secParam, const Db<Doc<>, Kw>& db) override;
};
