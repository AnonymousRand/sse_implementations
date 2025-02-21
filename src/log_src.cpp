#include <algorithm>
#include <random>

#include "log_src.h"

template <typename Underlying>
LogSrc<Underlying>::LogSrc(const Underlying& underlying) : underlying(underlying) {};

////////////////////////////////////////////////////////////////////////////////
// API Functions
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
void LogSrc<Underlying>::setup(int secParam, const Db<>& db) {
    this->key = this->underlying.genKey(secParam);

    // build index
    
    // need to find largest keyword: we can't pass in all the keywords raw, as leaves need to be contiguous
    Kw maxKw = -1;
    for (std::pair entry : db) {
        KwRange kwRange = entry.second;
        if (kwRange.second > maxKw) {
            maxKw = kwRange.second;
        }
    }
    this->tdag = TdagNode<Kw>::buildTdag(maxKw);

    // replicate every document to all keyword ranges/nodes in TDAG that cover it
    // temporarily use `unordered_map` (an inverted index) instead of `vector` to
    // easily identify which docs share the same `kwRange` for shuffling later
    std::unordered_map<KwRange, std::vector<Doc>> index;
    int temp = 0;
    for (std::pair entry : db) {
        Doc doc = entry.first;
        KwRange kwRange = entry.second;
        std::list<TdagNode<Kw>*> ancestors = this->tdag->getLeafAncestors(kwRange);
        for (TdagNode<Kw>* ancestor : ancestors) {
            KwRange ancestorKwRange = ancestor->getRange();
            if (index.count(ancestorKwRange) == 0) {
                index[ancestorKwRange] = std::vector {doc};
            } else {
                index[ancestorKwRange].push_back(doc);
            }
        }
    }

    // randomly permute documents associated with same keyword range/node and convert temporary `unordered_map` to `Db`
    Db<Doc> processedDb;
    std::random_device rd;
    std::mt19937 rng(rd());
    for (std::pair pair : index) {
        KwRange kwRange = pair.first;
        std::vector<Doc> docs = pair.second;
        std::shuffle(docs.begin(), docs.end(), rng);
        for (Doc doc : docs) {
            processedDb.push_back(std::pair {doc, kwRange});
        }
    }

    this->encInd = this->underlying.buildIndex(this->key, processedDb);
}

template <typename Underlying>
std::vector<Doc> LogSrc<Underlying>::search(const KwRange& query) {
    TdagNode<Kw>* src = this->tdag->findSrc(query);
    if (src == nullptr) { 
        return std::vector<Doc> {};
    }
    QueryToken queryToken = this->underlying.genQueryToken(this->key, src->getRange());
    return this->underlying.serverSearch(this->encInd, queryToken);
}

////////////////////////////////////////////////////////////////////////////////
// Template Instantiations
////////////////////////////////////////////////////////////////////////////////

template class LogSrc<PiBas>;
