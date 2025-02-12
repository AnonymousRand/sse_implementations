#include <algorithm>
#include <random>

#include "log_src.h"

////////////////////////////////////////////////////////////////////////////////
// Client
////////////////////////////////////////////////////////////////////////////////

template class LogSrcClient<PiBasClient>;

template <typename Underlying>
LogSrcClient<Underlying>::LogSrcClient(Underlying underlying)
        : IRangeSseClient<ustring, EncInd, Underlying>(underlying) {}

template <typename Underlying>
ustring LogSrcClient<Underlying>::setup(int secParam) {
    return this->underlying.setup(secParam);
}

template <typename Underlying>
EncInd LogSrcClient<Underlying>::buildIndex(ustring key, Db<> db) {
    // need to find largest keyword: we can't pass in all the keywords raw, as leaves need to be contiguous
    Kw maxKw = -1;
    for (auto entry : db) {
        KwRange kwRange = std::get<1>(entry);
        if (kwRange.second > maxKw) {
            maxKw = kwRange.second;
        }
    }
    this->tdag = TdagNode<Kw>::buildTdag(maxKw);

    // replicate every document to all keyword ranges/nodes in TDAG that cover it
    // temporarily use `unordered_map` instead of `vector` to easily identify
    // which docs share the same `kwRange` for shuffling later
    std::map<KwRange, std::vector<Id>> tempProcessedDb;
    for (auto entry : db) {
        Id id = std::get<0>(entry);
        KwRange kwRange = std::get<1>(entry);
        std::list<TdagNode<Kw>*> ancestors = this->tdag->getLeafAncestors(kwRange);
        for (TdagNode<Kw>* ancestor : ancestors) {
            KwRange ancestorKwRange = ancestor->getRange();
            if (tempProcessedDb.count(ancestorKwRange) == 0) {
                tempProcessedDb[ancestorKwRange] = std::vector<Id> {id};
            } else {
                tempProcessedDb[ancestorKwRange].push_back(id);
            }
        }
    }

    // randomly permute documents associated with same keyword range/node and convert temporary `map` to `Db`
    Db<> processedDb;
    std::random_device rd;
    std::mt19937 rng(rd());
    for (auto pair : tempProcessedDb) {
        KwRange kwRange = pair.first;
        std::vector<Id> ids = pair.second;
        std::shuffle(ids.begin(), ids.end(), rng);
        for (Id id : ids) {
            processedDb.push_back(Doc {id, kwRange});
        }
    }

    return this->underlying.buildIndexGeneric(key, processedDb);
}

template <typename Underlying>
QueryToken LogSrcClient<Underlying>::trpdr(ustring key, KwRange kwRange) {
    TdagNode<Kw>* src = this->tdag->findSrc(kwRange);
    return this->underlying.trpdrGeneric(key, src->getRange());
}

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

template class LogSrcServer<PiBasServer>;

template <typename Underlying>
LogSrcServer<Underlying>::LogSrcServer(Underlying underlying) : IRangeSseServer<Underlying>(underlying) {}

template <typename Underlying>
std::vector<Id> LogSrcServer<Underlying>::search(EncInd encInd, QueryToken queryToken) {
    return this->underlying.searchGeneric(encInd, queryToken);
}
