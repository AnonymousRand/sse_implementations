#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <typename Underlying = PiBas>
class LogSrci : public ISse {
    protected:
        const Underlying& underlying;
        std::pair<ustring, ustring> key;
        std::pair<EncInd, EncInd> encInds;
        TdagNode<Kw>* tdag1;
        TdagNode<Id>* tdag2;

    public:
        LogSrci(const Underlying& underlying);

        // API functions
        void setup(int secParam, const Db<>& db) override;
        std::vector<Doc> search(const KwRange& query) override;
};
