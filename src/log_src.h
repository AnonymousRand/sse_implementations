#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <typename DbDocType = Id, typename Underlying = PiBas<DbDocType>>
class LogSrc : public ISse<DbDocType> {
    private:
        const Underlying& underlying;
        ustring key;
        EncInd encInd;
        TdagNode<Kw>* tdag;

    public:
        LogSrc(const Underlying& underlying);

        // API functions
        void setup(int secParam, const Db<DbDocType>& db) override;
        std::vector<DbDocType> search(const KwRange& query) override;
};
