#include <algorithm>
#include <random>
#include <unordered_map>

#include "log_src.h"
#include "pi_bas.h"

////////////////////////////////////////////////////////////////////////////////
// LogSrcClient
////////////////////////////////////////////////////////////////////////////////

LogSrcClient::LogSrcClient(PiBasClient<ustring, EncInd>& underlying) : IRangeSseClient<ustring, EncInd>(underlying) {};

ustring LogSrcClient::setup(int secParam) {
    return this->underlying.setup(secParam);
}

EncInd LogSrcClient::buildIndex(ustring key, Db<Id, KwRange> db) {
    EncInd encInd;

    // build TDAG over keywords
    // need to find largest keyword: we can't pass in all the keywords raw, as leaves need to be contiguous
    Kw maxKw = -1;
    for (auto entry : db) {
        KwRange kwRange = std::get<1>(entry);
        if (kwRange.second > maxKw) {
            maxKw = kwRange.second;
        }
    }
    this->tdag = TdagNode<Kw>::buildTdag(maxKw);

    // replicate every document to all nodes/keywords ranges in TDAG that cover it
    // temporarily use `unordered_map` instead of `vector` to easily identify
    // which docs share the same `kwRange` for shuffling later
    std::unordered_map<DbDocType, std::vector<KwRange>> dbWithReplicas;
    for (auto entry : db) {
        Id id = std::get<0>(entry);
        KwRange kwRange = std::get<1>(entry);
        std::list<TdagNode*> ancestors = this->tdag->getLeafAncestors(kwRange);
        for (TdagNode* ancestor : ancestors) {
            KwRange ancestorKwRange = ancestor->getKwRange();
            if (dbWithReplicas.count(id) == 0) {
                dbWithReplicas[id] = std::vector<KwRange> {ancestorKwRange};
            } else {
                dbWithReplicas[id].push_back(ancestorKwRange);
            }
        }
    }

    // randomly permute documents associated with same keyword range/node and convert temporary `unordered_map` to `Db`
    Db<Id, KwRange> processedDb;
    std::random_device rd;
    std::mt19937 rng(rd());
    for (auto pair : dbWithReplicas) {
        Id id = pair.first;
        std::vector<KwRange> kwRanges = pair.second;
        std::shuffle(kwRanges.begin(), kwRanges.end(), rng);
        for (KwRange kwRange : kwRanges) {
            processedDb.push_back(Doc {id, kwRange});
        }
    }

    return this->underlying.buildIndexGeneric(key, processedDb);
}

QueryToken LogSrcClient::trpdr(ustring key, KwRange kwRange) {
    TdagNode* src = this->tdag->findSrc(kwRange);
    std::cout << "SRC for " << kwRange << " is " << src->getKwRange() << std::endl;
    return this->underlying.trpdr(key, src->getKwRange());
}

////////////////////////////////////////////////////////////////////////////////
// LogSrcServer
////////////////////////////////////////////////////////////////////////////////

LogSrcServer::LogSrcServer(ISseServer<EncInd>& underlying) : IRangeSseServer<EncInd>(underlying) {};

std::vector<Id> LogSrcServer::search(EncInd encInd, QueryToken queryToken) {
    return this->underlying.search(encInd, queryToken);
}
