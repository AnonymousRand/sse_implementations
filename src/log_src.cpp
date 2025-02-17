#include <algorithm>
#include <random>

#include "log_src.h"
#include "pi_bas.h"

////////////////////////////////////////////////////////////////////////////////
// Client
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
LogSrcClient<Underlying>::LogSrcClient(Underlying underlying) : IRangeSseClient<Underlying>(underlying) {}

template <typename Underlying>
ustring LogSrcClient<Underlying>::setup(int secParam) {
    return this->underlying.setup(secParam);
}

template <typename Underlying>
EncInd LogSrcClient<Underlying>::buildIndex(const ustring& key, const Db<>& db) {
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
    int temp = 0;
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
            processedDb.push_back(std::make_tuple(id, kwRange));
        }
    }

    return this->underlying.buildIndex(key, processedDb);
}

template <typename Underlying>
QueryToken LogSrcClient<Underlying>::trpdr(const ustring& key, const KwRange& kwRange) {
    TdagNode<Kw>* src = this->tdag->findSrc(kwRange);
    // if keyword not in TDAG/index; make sure server handles this and returns no results!
    if (src == nullptr) { 
        return this->underlying.trpdr(key, KwRange {-1, -1});
    }
    return this->underlying.trpdr(key, src->getRange());
}

template class LogSrcClient<PiBasClient>;

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
LogSrcServer<Underlying>::LogSrcServer(Underlying underlying) : IRangeSseServer<Underlying>(underlying) {}

template <typename Underlying>
std::vector<Id> LogSrcServer<Underlying>::search(const EncInd& encInd, const QueryToken& queryToken) {
    return this->underlying.search(encInd, queryToken);
}

template class LogSrcServer<PiBasServer>;
