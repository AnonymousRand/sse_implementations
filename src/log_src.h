#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <typename DbDoc = IdOp, typename Underlying = PiBas<DbDoc>>
class LogSrc : public ISse<DbDoc> {
    private:
        const Underlying& underlying;
        ustring key;
        EncInd encInd;
        TdagNode<Kw>* tdag;

    public:
        LogSrc(const Underlying& underlying);

        // API functions
        void setup(int secParam, const Db<DbDoc>& db) override;
        std::vector<DbDoc> search(const KwRange& query) override;
};
