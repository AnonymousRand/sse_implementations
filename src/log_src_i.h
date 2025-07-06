#pragma once


#include "sse.h"
#include "utils/tdag.h"


/******************************************************************************/
/* `LogSrcIBase`                                                              */
/******************************************************************************/


// common code between `LogSrcI` and `LogSrcILoc`
template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
class LogSrcIBase : public ISdaUnderlySse<Doc, Kw> {
    public:
        LogSrcIBase();
        virtual ~LogSrcIBase();

        std::vector<Doc> search(
            const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
        ) const override;

        void clear() override;
        Db<Doc, Kw> getDb() const override;
        bool isEmpty() const override;

    protected:
        Underly<SrcIDb1Doc, Kw>* underly1 = nullptr;
        Underly<Doc, IdAlias>* underly2 = nullptr;
        TdagNode<Kw>* tdag1 = nullptr;
        TdagNode<IdAlias>* tdag2 = nullptr;
        Db<Doc, Kw> db; // store since neither underlying instance contains the original DB
        bool _isEmpty = false;
};


/******************************************************************************/
/* `LogSrcI`                                                                  */
/******************************************************************************/


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
class LogSrcI : public LogSrcIBase<Underly> {
    public:
        LogSrcI();

        /**
         * Precondition:
         *     - Entries in `db` must have size 1 `Kw` ranges, i.e. a singular `Kw` value.
         *     - Entries in `db` cannot have keyword equal to `DUMMY`.
         */
        void setup(int secParam, const Db<Doc, Kw>& db) override;
};
