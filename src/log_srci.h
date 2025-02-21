#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <typename DbDoc = IdOp, typename Underlying = PiBas<DbDoc>>
class LogSrci : public ISse<DbDoc> {
    protected:
        const Underlying& underlying;
        std::pair<ustring, ustring> key;
        std::pair<EncInd, EncInd> encInds;
        TdagNode<Kw>* tdag1;
        TdagNode<DbDoc>* tdag2;

    public:
        LogSrci(const Underlying& underlying);

        // API functions
        void setup(int secParam, const Db<DbDoc>& db) override;
        std::vector<DbDoc> search(const KwRange& query) override;
};
