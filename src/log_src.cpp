#include <algorithm>
#include <random>

#include "log_src.h"

template <typename DbDoc, Underlying>
LogSrc<DbDoc, Underlying>::LogSrc(const Underlying& underlying) : underlying(underlying) {};

////////////////////////////////////////////////////////////////////////////////
// API Functions
////////////////////////////////////////////////////////////////////////////////

template <typename DbDoc, Underlying>
void LogSrc<DbDoc, Underlying>::setup(int secParam, const Db<DbDoc>& db) {
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
    // temporarily use `unordered_map` (an inverted index) instead of `vector` to
    // easily identify which docs share the same `kwRange` for shuffling later
    std::unordered_map<KwRange, std::vector<DbDoc>> index;
    int temp = 0;
    for (auto entry : db) {
        DbDoc dbDoc = std::get<0>(entry);
        KwRange kwRange = std::get<1>(entry);
        std::list<TdagNode<Kw>*> ancestors = this->tdag->getLeafAncestors(kwRange);
        for (TdagNode<Kw>* ancestor : ancestors) {
            KwRange ancestorKwRange = ancestor->getRange();
            if (index.count(ancestorKwRange) == 0) {
                index[ancestorKwRange] = std::vector {dbDoc};
            } else {
                index[ancestorKwRange].push_back(dbDoc);
            }
        }
    }

    // randomly permute documents associated with same keyword range/node and convert temporary `unordered_map` to `Db`
    Db<DbDoc> processedDb;
    std::random_device rd;
    std::mt19937 rng(rd());
    for (auto pair : index) {
        KwRange kwRange = pair.first;
        std::vector<DbDoc> dbDocs = pair.second;
        std::shuffle(dbDocs.begin(), dbDocs.end(), rng);
        for (DbDoc dbDoc : dbDocs) {
            processedDb.push_back(std::pair {dbDoc, kwRange});
        }
    }

    this->encInd = this->underlying.buildIndex(this->key, processedDb);
}

template <typename DbDoc, Underlying>
std::vector<DbDoc> LogSrc<DbDoc, Underlying>::search(const KwRange& query) {
    TdagNode<Kw>* src = this->tdag->findSrc(query);
    if (src == nullptr) { 
        return std::vector<DbDoc> {};
    }
    QueryToken queryToken = this->underlying.genQueryToken(this->key, src->getRange());
    return this->underlying.serverSearch(this->encInd, queryToken);
}

////////////////////////////////////////////////////////////////////////////////
// Template Instantiations
////////////////////////////////////////////////////////////////////////////////

template class LogSrc<Id, PiBas<Id>>;
template class LogSrc<IdOp, PiBas<IdOp>>;
