#include <algorithm>
#include <random>

#include "log_src.h"

template <typename DbDocType, typename Underlying>
LogSrc<DbDocType, Underlying>::LogSrc(const Underlying& underlying) : underlying(underlying) {};

////////////////////////////////////////////////////////////////////////////////
// API Functions
////////////////////////////////////////////////////////////////////////////////

template <typename DbDocType, typename Underlying>
void LogSrc<DbDocType, Underlying>::setup(int secParam, const Db<DbDocType>& db) {
    this->key = this->underlying.genKey(secParam);

    // build index
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
    std::unordered_map<KwRange, std::vector<Id>> tempProcessedDb;
    int temp = 0;
    for (auto entry : db) {
        Id id = std::get<0>(entry);
        KwRange kwRange = std::get<1>(entry);
        std::list<TdagNode<Kw>*> ancestors = this->tdag->getLeafAncestors(kwRange);
        for (TdagNode<Kw>* ancestor : ancestors) {
            KwRange ancestorKwRange = ancestor->getRange();
            if (tempProcessedDb.count(ancestorKwRange) == 0) {
                tempProcessedDb[ancestorKwRange] = std::vector {id};
            } else {
                tempProcessedDb[ancestorKwRange].push_back(id);
            }
        }
    }

    // randomly permute documents associated with same keyword range/node and convert temporary `unordered_map` to `Db`
    Db<> processedDb;
    std::random_device rd;
    std::mt19937 rng(rd());
    for (auto pair : tempProcessedDb) {
        KwRange kwRange = pair.first;
        std::vector<Id> ids = pair.second;
        std::shuffle(ids.begin(), ids.end(), rng);
        for (Id id : ids) {
            processedDb.push_back(std::tuple {id, kwRange});
        }
    }

    this->encInd = this->underlying.buildIndex(this->key, processedDb);
}

template <typename DbDocType, typename Underlying>
std::vector<DbDocType> LogSrc<DbDocType, Underlying>::search(const KwRange& query) {
    TdagNode<Kw>* src = this->tdag->findSrc(query);
    if (src == nullptr) { 
        return std::vector<DbDocType> {};
    }
    QueryToken queryToken = this->underlying.genQueryToken(this->key, src->getRange());
    return this->underlying.template serverSearch<DbDocType>(this->encInd, queryToken);
}

////////////////////////////////////////////////////////////////////////////////
// Template Instantiations
////////////////////////////////////////////////////////////////////////////////

template class LogSrc<Id, PiBas<Id>>;
