#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <typename Underlying = PiBas>
class LogSrc : public ISse {
    private:
        const Underlying& underlying;
        ustring key;
        EncInd encInd;
        TdagNode<Kw>* tdag;

    public:
        LogSrc(const Underlying& underlying);

        // API functions
        void setup(int secParam, const Db<>& db) override;
        std::vector<Id> search(const KwRange& query) override;
};
