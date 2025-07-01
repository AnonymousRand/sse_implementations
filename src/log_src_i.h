#pragma once


#include "sse.h"
#include "util/tdag.h"


/******************************************************************************/
/* `LogSrcIBase`                                                              */

/******************************************************************************/


// common code between `LogSrcI` and `LogSrcILoc`
template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
class LogSrcIBase : public ISse<Doc, Kw>, public ISdaUnderly<Doc, Kw> {
    protected:
        Underly<SrcIDb1Doc, Kw>* underly1 = nullptr;
        Underly<Doc, IdAlias>* underly2 = nullptr;
        TdagNode<Kw>* tdag1 = nullptr;
        TdagNode<IdAlias>* tdag2 = nullptr;
        Db<Doc, Kw> db; // store since neither underlying instance contains the original DB
        bool _isEmpty = false;

        LogSrcIBase(Underly<SrcIDb1Doc, Kw>* underly1, Underly<Doc, IdAlias>* underly2, EncIndType encIndType);

    public:
        LogSrcIBase();
        LogSrcIBase(EncIndType encIndType);
        virtual ~LogSrcIBase();

        std::vector<Doc> search(
            const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
        ) const override;

        void clear() override;
        Db<Doc, Kw> getDb() const override;
        bool isEmpty() const override;
        void setEncIndType(EncIndType encIndType) override;
};


/******************************************************************************/
/* `LogSrcI`                                                                  */

/******************************************************************************/


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
class LogSrcI : public LogSrcIBase<Underly> {
    public:
        LogSrcI();
        LogSrcI(EncIndType encIndType);

        void setup(int secParam, const Db<Doc, Kw>& db) override;
};
