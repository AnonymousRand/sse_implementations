#include <algorithm>
#include <random>
#include <unordered_map>

#include "log_src.h"
#include "pi_bas.h"

////////////////////////////////////////////////////////////////////////////////
// LogSrcClient
////////////////////////////////////////////////////////////////////////////////

LogSrcClient::LogSrcClient(SseClient<ustring, EncInd>& underlying)
        : RangeSseClient<ustring, EncInd>(underlying) {};

ustring LogSrcClient::setup(int secParam) {
    return this->underlying.setup(secParam);
}

EncInd LogSrcClient::buildIndex(ustring key, Db db) {
    EncInd encInd;

    // build TDAG over keywords
    // need to find largest keyword: we can't pass in all the keywords raw, as leaves need to be contiguous
    Kw maxKw = -1;
    for (Doc doc : db) {
        KwRange kwRange = std::get<1>(doc);
        if (kwRange.end > maxKw) {
            maxKw = kwRange.end;
        }
    }
    this->tdag = TdagNode::buildTdag(maxKw);

    // replicate every document to all nodes/keywords ranges in TDAG that cover it
    // temporarily use `unordered_map` to easily identify which docs share the same `kwRange` for shuffling later
    std::unordered_map<Id, std::vector<KwRange>> dbWithReplicas;
    for (Doc doc : db) {
        Id id = std::get<0>(doc);
        KwRange kwRange = std::get<1>(doc);
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

    // randomly permute documents associated with same keyword range/node and convert temporary `unordered_map` to `Db`
    Db processedDb;
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

    return this->underlying.buildIndex(key, processedDb);
}

QueryToken LogSrcClient::trpdr(ustring key, KwRange kwRange) {
    TdagNode* src = this->tdag->findSrc(kwRange);
    std::cout << "SRC for " << kwRange << " is " << src->getKwRange() << std::endl;
    return this->underlying.trpdr(key, src->getKwRange());
}

////////////////////////////////////////////////////////////////////////////////
// LogSrcServer
////////////////////////////////////////////////////////////////////////////////

LogSrcServer::LogSrcServer(SseServer<EncInd>& underlying) : RangeSseServer<EncInd>(underlying) {};

std::vector<Id> LogSrcServer::search(EncInd encInd, QueryToken queryToken) {
    return this->underlying.search(encInd, queryToken);
}
