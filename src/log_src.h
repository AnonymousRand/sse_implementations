#pragma once

#include "pi_bas.h"
#include "sse.h"
#include "util/tdag.h"

template <typename DbDocType = Id>
class LogSrc : public ISse<DbDocType>, public IRangeSse<PiBas<DbDocType>> {
    private:
        ustring key;
        EncInd encInd;
        TdagNode<Kw>* tdag;

    public:
        LogSrc(const PiBas<DbDocType>& underlying);

        // API functions
        void setup(int secParam, const Db<DbDocType>& db) override;
        std::vector<DbDocType> search(const KwRange& query) override;
};
