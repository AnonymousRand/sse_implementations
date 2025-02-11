#include <algorithm>
#include <random>
#include <unordered_map>

#include "log_src.h"
#include "pi_bas.h"

////////////////////////////////////////////////////////////////////////////////
// LogSrcClient
////////////////////////////////////////////////////////////////////////////////

LogSrcClient::LogSrcClient(ISseClient<ustring, EncInd>& underlying)
        : IRangeSseClient<ustring, EncInd>(underlying) {};

ustring LogSrcClient::setup(int secParam) {
    return this->underlying.setup(secParam);
}

EncInd LogSrcClient::buildIndex(ustring key, Db<Id> db) {
    EncInd encInd;

    // build TDAG over keywords
    // need to find largest keyword: we can't pass in all the keywords raw, as leaves need to be contiguous
    Range maxKw = -1;
    for (auto entry : db) {
        Range kw = std::get<1>(entry);
        if (kw.second > maxKw) {
            maxKw = kwRange.second;
        }
    }
    this->tdag = TdagNode::buildTdag(maxKw);

    // replicate every document to all nodes/keywords ranges in TDAG that cover it
    // temporarily use `unordered_map` instead of `vector` to easily identify
    // which docs share the same `kwRange` for shuffling later
    std::unordered_map<DbDocType, std::vector<Range>> dbWithReplicas;
    for (auto entry : db) {
        DbDocType doc = std::get<0>(entry);
        Range kw = std::get<1>(entry);
        std::list<TdagNode*> ancestors = this->tdag->getLeafAncestors(kw);
        for (TdagNode* ancestor : ancestors) {
            Range ancestorKw = ancestor->getKwRange();
            if (dbWithReplicas.count(doc) == 0) {
                dbWithReplicas[doc] = std::vector<Range> {ancestorKw};
            } else {
                dbWithReplicas[doc].push_back(ancestorKw);
            }
        }
    }

    // randomly permute documents associated with same keyword range/node and convert temporary `unordered_map` to `Db`
    Db<> processedDb;
    std::random_device rd;
    std::mt19937 rng(rd());
    for (auto pair : dbWithReplicas) {
        DbDocType doc = pair.first;
        std::vector<Range> kws = pair.second;
        std::shuffle(kws.begin(), kws.end(), rng);
        for (Range kw : kws) {
            processedDb.push_back(Doc {doc, kw});
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

LogSrcServer::LogSrcServer(ISseServer<EncInd>& underlying) : IRangeSseServer<EncInd>(underlying) {};

std::vector<Id> LogSrcServer::search(EncInd encInd, QueryToken queryToken) {
    return this->underlying.search(encInd, queryToken);
}
