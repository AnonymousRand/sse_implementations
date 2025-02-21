#include <algorithm>
#include <random>

#include <openssl/rand.h>

#include "log_srci.h"
#include "pi_bas.h"
#include "util/openssl.h"

template <typename Underlying>
LogSrci<Underlying>::LogSrci(const Underlying& underlying) : underlying(underlying) {}

////////////////////////////////////////////////////////////////////////////////
// API Functions
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
void LogSrci<Underlying>::setup(int secParam, const Db<>& db) {
    ustring key1 = this->underlying.genKey(secParam);
    ustring key2 = this->underlying.genKey(secParam);
    this->key = std::pair {key1, key2};

    // build index 1

    // build TDAG1 over keywords
    Kw maxKw = -1;
    for (auto entry : db) {
        const KwRange& kwRange = std::get<1>(entry);
        if (kwRange.second > maxKw) {
            maxKw = kwRange.second;
        }
    }
    this->tdag1 = TdagNode<Kw>::buildTdag(maxKw);

    // first figure out which documents share the same keywords by building index and list of unique kws like in PiBas
    std::unordered_map<KwRange, std::set<Id>> index;
    std::set<KwRange> uniqueKwRanges;
    for (auto entry : db) {
        Id id = std::get<0>(entry);
        const KwRange& kwRange = std::get<1>(entry);

        if (index.count(kwRange) == 0) {
            index[kwRange] = std::set {id};
        } else {
            index[kwRange].insert(id);
        }
        uniqueKwRanges.insert(kwRange);
    }

    // replicate every document (in this case pairs) to all keyword ranges/nodes in TDAG1 that cover it
    // thus `db1` contains the (inverted) pairs used to build index 1: ((kw, id range), kw range/node)
    Db<SrciDb1Doc, KwRange> db1;
    for (const KwRange& kwRange : uniqueKwRanges) {
        auto itDocsWithSameKwRange = index.find(kwRange);
        std::set<Id> docsWithSameKwRange = itDocsWithSameKwRange->second;
        IdRange idRange {*docsWithSameKwRange.begin(), *docsWithSameKwRange.rbegin()}; // `set` moment
        SrciDb1Doc pair {kwRange, idRange};

        std::list<TdagNode<Kw>*> ancestors = this->tdag1->getLeafAncestors(kwRange);
        for (TdagNode<Kw>* ancestor : ancestors) {
            std::pair<SrciDb1Doc, KwRange> finalEntry {pair, ancestor->getRange()};
            db1.push_back(finalEntry);
        }
    }

    // build index 2

    // build TDAG2 over documents
    Id maxId = -1;
    for (auto entry : db) {
        Id id = std::get<0>(entry);
        if (id > maxId) {
            maxId = id;
        }
    }
    this->tdag2 = TdagNode<Id>::buildTdag(maxId);

    // replicate every document to all id ranges/nodes in TDAG2 that cover it
    // again need temporary `unordered_map` to shuffle
    std::unordered_map<IdRange, std::vector<Id>> tempInd2;
    for (auto entry : db) {
        Id id = std::get<0>(entry);
        std::list<TdagNode<Id>*> ancestors = this->tdag2->getLeafAncestors(IdRange {id, id});
        for (TdagNode<Id>* ancestor : ancestors) {
            IdRange ancestorIdRange = ancestor->getRange();
            if (tempInd2.count(ancestorIdRange) == 0) {
                tempInd2[ancestorIdRange] = std::vector {id};
            } else {
                tempInd2[ancestorIdRange].push_back(id);
            }
        }
    }

    // randomly permute documents associated with same id range/node and convert temporary `unordered_map` to `Db`
    Db<Id, IdRange> db2;
    std::random_device rd;
    std::mt19937 rng(rd());
    for (auto pair : tempInd2) {
        IdRange idRange = pair.first;
        std::vector<Id> ids = pair.second;
        std::shuffle(ids.begin(), ids.end(), rng);
        for (Id id: ids) {
            db2.push_back(std::pair {id, idRange});
        }
    }

    EncInd encInd1 = this->underlying.buildIndex(key1, db1);
    EncInd encInd2 = this->underlying.buildIndex(key2, db2);
    this->encInds = std::pair {encInd1, encInd2};
}

template <typename Underlying>
std::vector<Id> LogSrci<Underlying>::search(const KwRange& query) {
    // query 1

    TdagNode<Kw>* src1 = this->tdag1->findSrc(query);
    if (src1 == nullptr) { 
        return std::vector<Id> {};
    }
    QueryToken queryToken1 = this->underlying.genQueryToken(this->key.first, src1->getRange());
    std::vector<SrciDb1Doc> choices = this->underlying.template serverSearch<SrciDb1Doc>(this->encInds.first, queryToken1);

    // query 2

    Id minId = -1, maxId = -1;
    // filter out unnecessary choices and merge remaining ones into a single id range
    for (SrciDb1Doc choice : choices) {
        KwRange choiceKwRange = choice.get().first;
        if (query.contains(choiceKwRange)) {
            IdRange choiceIdRange = choice.get().second;
            if (choiceIdRange.first < minId || minId == -1) {
                minId = choiceIdRange.first;
            }
            if (choiceIdRange.second > maxId) {
                maxId = choiceIdRange.second;
            }
        }
    }

    IdRange idRangeToQuery {minId, maxId};
    TdagNode<Id>* src2 = this->tdag2->findSrc(idRangeToQuery);
    if (src2 == nullptr) { 
        return std::vector<Id> {};
    }
    QueryToken queryToken2 = this->underlying.genQueryToken(this->key.second, src2->getRange());
    return this->underlying.serverSearch(this->encInds.second, queryToken2);
}

////////////////////////////////////////////////////////////////////////////////
// Template Instantiations
////////////////////////////////////////////////////////////////////////////////

template class LogSrci<PiBas>;
