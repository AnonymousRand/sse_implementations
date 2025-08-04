#pragma once


#include "pi_bas.h"
#include "utils/tdag.h"


/******************************************************************************/
/* `LogSrcIBase`                                                              */
/******************************************************************************/


// common code between `LogSrcI` and `LogSrcILoc`
template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
class LogSrcIBase : public ISdaUnderlySse<Doc<>, Kw> {
    public:
        LogSrcIBase();
        virtual ~LogSrcIBase();

        //----------------------------------------------------------------------
        // `ISse`

        std::vector<Doc<>> search(
            const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
        ) const override;
        void clear() override;

        //----------------------------------------------------------------------
        // `ISdaUnderlySse`

        void getDb(Db<Doc<>, Kw>& ret) const override;

    protected:
        Underly<SrcIDb1Doc, Kw>* underly1 = nullptr;
        Underly<Doc<IdAlias>, IdAlias>* underly2 = nullptr;
        // this is only used to store the original db in `getDb()` so that it is encrypted but easy to recover
        // instead of reconstructing the original db from `underly1`'s and `underly2`'s indexes/dbs
        // do NOT search on this one!
        // also it's specifically PiBas since `LogSrcILoc` may use locality-aware `Underly`,
        // which doesn't store non-TDAG-structured datasets correctly
        PiBas<Doc<>, Kw>* origDbUnderly = nullptr;
        TdagNode<Kw>* tdag1 = nullptr;
        TdagNode<IdAlias>* tdag2 = nullptr;
};


/******************************************************************************/
/* `LogSrcI`                                                                  */
/******************************************************************************/


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
class LogSrcI : public LogSrcIBase<Underly> {
    public:
        LogSrcI();

        //----------------------------------------------------------------------
        // `ISse`

        /**
         * Precondition:
         *     - Entries in `db` must have size 1 `Kw` ranges, i.e. a singular `Kw` value.
         *     - Entries in `db` cannot have keyword equal to `DUMMY`.
         */
        void setup(int secParam, const Db<Doc<>, Kw>& db) override;
};
