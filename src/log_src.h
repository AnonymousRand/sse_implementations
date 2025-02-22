#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <typename Underlying = PiBas>
class LogSrc : public ISse {
    private:
        Underlying& underlying;
        ustring key;
        EncInd encInd;
        TdagNode<Kw>* tdag;

    public:
        LogSrc(Underlying& underlying);

        // API functions
        void setup(int secParam, const Db<>& db) override;
        std::vector<Doc> search(const KwRange& query) override;
};
