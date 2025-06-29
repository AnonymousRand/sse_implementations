#pragma once


#include "sse.h"
#include "util/tdag.h"


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
class LogSrcI : public ISse<Doc, Kw>, public ISdaUnderly<Doc, Kw> {
    private:
        Underly<SrcIDb1Doc, Kw>* underly1 = nullptr;
        Underly<Doc, IdAlias>* underly2 = nullptr;
        TdagNode<Kw>* tdag1 = nullptr;
        TdagNode<IdAlias>* tdag2 = nullptr;
        Db<Doc, Kw> db; // store since neither underlying instance contains the original DB
        bool _isEmpty = false;

        LogSrcI(Underly<SrcIDb1Doc, Kw>* underly1, Underly<Doc, IdAlias>* underly2, EncIndType encIndType);

    public:
        LogSrcI();
        LogSrcI(EncIndType encIndType);
        ~LogSrcI();

        void setup(int secParam, const Db<Doc, Kw>& db) override;
        std::vector<Doc> search(
            const Range<Kw>& query, bool shouldProcessResults = true, bool isNaive = true
        ) const override;

        void clear() override;
        Db<Doc, Kw> getDb() const override;
        bool isEmpty() const override;
        void setEncIndType(EncIndType encIndType) override;
};
