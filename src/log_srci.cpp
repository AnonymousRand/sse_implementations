#include <algorithm>
#include <random>

#include <openssl/rand.h>

#include "log_srci.h"
#include "pi_bas.h"
#include "util/openssl.h"

template <typename Underlying>
LogSrci<Underlying>::LogSrci(Underlying& underlying) : underlying(underlying) {}

template <typename Underlying>
void LogSrci<Underlying>::setup(int secParam, const Db<>& db) {
    ustring key1 = this->underlying.genKey(secParam);
    ustring key2 = this->underlying.genKey(secParam);
    this->key = std::pair {key1, key2};

    // build index 1

    // build TDAG1 over keywords
    Kw maxKw = -1;
    for (std::pair entry : db) {
        KwRange kwRange = entry.second;
        if (kwRange.second > maxKw) {
            maxKw = kwRange.second;
        }
    }
    this->tdag1 = TdagNode<Kw>::buildTdag(maxKw);

    // first figure out which documents share the same keywords by building index and list of unique kws like in PiBas
    std::unordered_map<KwRange, std::set<Id>> ind1;
    std::unordered_set<KwRange> uniqueKwRanges;
    for (std::pair entry : db) {
        Doc doc = entry.first;
        Id id = doc.get().first;
        KwRange kwRange = entry.second;

        if (ind1.count(kwRange) == 0) {
            ind1[kwRange] = std::set {id};
        } else {
            ind1[kwRange].insert(id);
        }
        uniqueKwRanges.insert(kwRange);
    }

    // replicate every document (in this case pairs) to all keyword ranges/nodes in TDAG1 that cover it
    // thus `db1` contains the (inverted) pairs used to build index 1: ((kw, [ids]), kw range/node)
    Db<SrciDb1Doc, KwRange> db1;
    for (KwRange kwRange : uniqueKwRanges) {
        auto itDocsWithSameKwRange = ind1.find(kwRange);
        std::set<Id> sameKwRangeIds = itDocsWithSameKwRange->second;
        IdRange idRange {*sameKwRangeIds.begin(), *sameKwRangeIds.rbegin()}; // this is why we used `set`
        SrciDb1Doc pair {kwRange, idRange};

        std::list<TdagNode<Kw>*> ancestors = this->tdag1->getLeafAncestors(kwRange);
        for (TdagNode<Kw>* ancestor : ancestors) {
            std::pair<SrciDb1Doc, KwRange> finalEntry {pair, ancestor->getRange()};
            db1.push_back(finalEntry);
        }
    }

    // build index 2

    // build TDAG2 over document ids
    Id maxId = Id::getMin();
    for (std::pair entry : db) {
        Doc doc = entry.first;
        Id id = doc.get().first;
        if (id > maxId) {
            maxId = id;
        }
    }
    this->tdag2 = TdagNode<Id>::buildTdag(maxId);

    // replicate every document to all id ranges/nodes in TDAG2 that cover it
    // again need temporary `unordered_map` index to shuffle
    std::unordered_map<IdRange, std::vector<Doc>> ind2;
    for (std::pair entry : db) {
        Doc doc = entry.first;
        Id id = doc.get().first;
        std::list<TdagNode<Id>*> ancestors = this->tdag2->getLeafAncestors(Range<Id> {id, id});
        for (TdagNode<Id>* ancestor : ancestors) {
            Range<Id> ancestorIdRange = ancestor->getRange();
            if (ind2.count(ancestorIdRange) == 0) {
                ind2[ancestorIdRange] = std::vector {doc};
            } else {
                ind2[ancestorIdRange].push_back(doc);
            }
        }
    }

    // randomly permute documents associated with same id range/node and convert temporary `unordered_map` to `Db`
    Db<Doc, IdRange> db2;
    std::random_device rd;
    std::mt19937 rng(rd());
    for (std::pair pair : ind2) {
        IdRange idRange = pair.first;
        std::vector<Doc> docs = pair.second;
        std::shuffle(docs.begin(), docs.end(), rng);
        for (Doc doc: docs) {
            db2.push_back(std::pair {doc, idRange});
        }
    }

    EncInd encInd1 = this->underlying.buildIndex(key1, db1);
    EncInd encInd2 = this->underlying.buildIndex(key2, db2);
    this->encInds = std::pair {encInd1, encInd2};
}

template <typename Underlying>
std::vector<Doc> LogSrci<Underlying>::search(const KwRange& query) {
    // query 1

    TdagNode<Kw>* src1 = this->tdag1->findSrc(query);
    if (src1 == nullptr) { 
        return std::vector<Doc> {};
    }
    QueryToken queryToken1 = this->underlying.genQueryToken(this->key.first, src1->getRange());
    std::vector<SrciDb1Doc> choices =
            this->underlying.template genericSearch<SrciDb1Doc>(this->encInds.first, queryToken1);

    // query 2

    Id minId = Id::getMin(), maxId = Id::getMin();
    // filter out unnecessary choices and merge remaining ones into a single id range
    for (SrciDb1Doc choice : choices) {
        KwRange choiceKwRange = choice.get().first;
        if (query.contains(choiceKwRange)) {
            IdRange choiceIdRange = choice.get().second;
            if (choiceIdRange.first < minId || minId == Id::getMin()) {
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
        return std::vector<Doc> {};
    }
    QueryToken queryToken2 = this->underlying.genQueryToken(this->key.second, src2->getRange());
    return this->underlying.search(this->encInds.second, queryToken2);
}

template <typename Underlying>
LogSrci<Underlying>& LogSrci<Underlying>::operator =(const LogSrci<Underlying>& other) {
    if (this == &other) {
        return *this;
    }
    this->underlying = other.underlying;
    return *this;
}

template class LogSrci<PiBas>;
