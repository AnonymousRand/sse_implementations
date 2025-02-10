#include <algorithm>
#include <random>

#include "log_src.h"
#include "pi_bas.h"

////////////////////////////////////////////////////////////////////////////////
// LogSrcClient
////////////////////////////////////////////////////////////////////////////////

LogSrcClient::LogSrcClient() {
    this->underlying = PiBasClient();
}

EncInd LogSrcClient::buildIndex(ustring key, Db db) {
    EncInd encInd;

    // build TDAG over keywords
    // need to find largest keyword: we can't pass in all the keywords raw, as leaves need to be contiguous
    Kw maxKw = -1;
    for (auto pair : db) {
        KwRange kwRange = pair.second;
        if (kwRange.end > maxKw) {
            maxKw = kwRange.end;
        }
    }
    this->tdag = TdagNode::buildTdag(maxKw);

    // replicate every document to all nodes/keywords ranges in TDAG that cover it
    std::unordered_map<Id, std::vector<KwRange>> dbWithReplicas;
    for (auto pair : db) {
        Id id = pair.first;
        KwRange kwRange = pair.second;
        std::list<TdagNode*> ancestors = this->tdag->getLeafAncestors(kwRange);
        for (TdagNode* ancestor : ancestors) {
            KwRange kwRange = ancestor->getKwRange();
            if (dbWithReplicas.count(id) == 0) {
                dbWithReplicas[id] = std::vector<KwRange> {kwRange};
            } else {
                dbWithReplicas[id].push_back(kwRange);
            }
        }
    }

    // randomly permute documents associated with same keyword range/node
    // then convert to `std::unordered_multimap` format compatible with `PiBasClient.buildIndex()`
    Db processedDb;
    std::random_device rd;
    std::mt19937 rng(rd());
    for (auto pair : dbWithReplicas) {
        Id id = pair.first;
        std::vector<KwRange> kwRanges = pair.second;
        std::shuffle(kwRanges.begin(), kwRanges.end(), rng);
        for (KwRange kwRange : kwRanges) {
            processedDb.insert(std::make_pair(id, kwRange));
        }
    }

    return this->underlying.buildIndex(key, processedDb);
}

QueryToken LogSrcClient::trpdr(ustring key, KwRange kwRange) {
    TdagNode* src = this->tdag->findSrc(kwRange);
    std::cout << "SRC for " << kwRange << " is " << src->getKwRange() << std::endl;
    return this->underlying.trpdr(key, src->getKwRange());
}
