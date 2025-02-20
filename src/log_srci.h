#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <typename DbDocType = Id, typename Underlying = PiBas<DbDocType>>
class LogSrci : public ISse<DbDocType> {
    protected:
        const Underlying& underlying;
        std::pair<ustring, ustring> key;
        std::pair<EncInd, EncInd> encInds;
        TdagNode<Kw>* tdag1;
        TdagNode<Id>* tdag2;

    public:
        LogSrci(const Underlying& underlying);

        // API functions
        void setup(int secParam, const Db<DbDocType>& db) override;
        std::vector<DbDocType> search(const KwRange& query) override;
};
