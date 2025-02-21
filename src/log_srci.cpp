#include <algorithm>
#include <random>

#include <openssl/rand.h>

#include "log_srci.h"
#include "pi_bas.h"
#include "util/openssl.h"

template <typename DbDoc, Underlying>
LogSrci<DbDoc, Underlying>::LogSrci(const Underlying& underlying) : underlying(underlying) {}

////////////////////////////////////////////////////////////////////////////////
// API Functions
////////////////////////////////////////////////////////////////////////////////

template <typename DbDoc, Underlying>
void LogSrci<DbDoc, Underlying>::setup(int secParam, const Db<DbDoc>& db) {
    ustring key1 = this->underlying.genKey(secParam);
    ustring key2 = this->underlying.genKey(secParam);
    this->key = std::pair {key1, key2};

    // build index 1

    // build TDAG1 over keywords
    Kw maxKw = -1;
    for (auto entry : db) {
        KwRange kwRange = std::get<1>(entry);
        if (kwRange.second > maxKw) {
            maxKw = kwRange.second;
        }
    }
    this->tdag1 = TdagNode<Kw>::buildTdag(maxKw);

    // first figure out which documents share the same keywords by building index and list of unique kws like in PiBas
    std::unordered_map<KwRange, std::set<DbDoc>> ind1;
    std::unordered_set<KwRange> uniqueKwRanges;
    for (auto entry : db) {
        DbDoc dbDoc = std::get<0>(entry);
        KwRange kwRange = std::get<1>(entry);

        if (index.count(kwRange) == 0) {
            ind1[kwRange] = std::set {dbDoc};
        } else {
            ind1[kwRange].insert(dbDoc);
        }
        uniqueKwRanges.insert(kwRange);
    }

    // replicate every document (in this case pairs) to all keyword ranges/nodes in TDAG1 that cover it
    // thus `db1` contains the (inverted) pairs used to build index 1: ((kw, [docs]), kw range/node)
    Db<SrciDb1Doc> db1;
    for (KwRange kwRange : uniqueKwRanges) {
        auto itDocsWithSameKwRange = ind1.find(kwRange);
        std::set<DbDoc> docsWithSameKwRange = itDocsWithSameKwRange->second;
        Range<DbDoc> dbDocRange {*docsWithSameKwRange.begin(), *docsWithSameKwRange.rbegin()}; // `set` moment
        SrciDb1Doc pair {kwRange, dbDocRange};

        std::list<TdagNode<Kw>*> ancestors = this->tdag1->getLeafAncestors(kwRange);
        for (TdagNode<Kw>* ancestor : ancestors) {
            std::pair<SrciDb1Doc, KwRange> finalEntry {pair, ancestor->getRange()};
            db1.push_back(finalEntry);
        }
    }

    // build index 2

    // build TDAG2 over documents
    DbDoc maxDbDoc = DbDoc {-1, INSERT};
    for (auto entry : db) {
        DbDoc dbDoc = std::get<0>(entry);
        if (dbDoc > maxDbDoc) {
            maxDbDoc = dbDoc;
        }
    }
    this->tdag2 = TdagNode<DbDoc>::buildTdag(maxDbDoc);

    // replicate every document to all id ranges/nodes in TDAG2 that cover it
    // again need temporary `unordered_map` index to shuffle
    std::unordered_map<IdRange, std::vector<DbDoc>> ind2;
    for (auto entry : db) {
        DbDoc dbDoc = std::get<0>(entry).first;
        std::list<TdagNode<DbDoc>*> ancestors = this->tdag2->getLeafAncestors(DbDocRange {dbDoc, dbDoc});
        for (TdagNode<DbDoc>* ancestor : ancestors) {
            Range<DbDoc> ancestorDbDocRange = ancestor->getRange();
            IdRange ancestorIdRange = toIdRange(ancestorDbDocRange);
            if (ind2.count(ancestorIdRange) == 0) {
                ind2[ancestorIdRange] = std::vector {dbDoc};
            } else {
                ind2[ancestorIdRange].push_back(dbDoc);
            }
        }
    }

    // randomly permute documents associated with same id range/node and convert temporary `unordered_map` to `Db`
    Db<DbDoc, IdRange> db2;
    std::random_device rd;
    std::mt19937 rng(rd());
    for (auto pair : ind2) {
        IdRange idRange = pair.first;
        std::vector<DbDoc> dbDocs = pair.second;
        std::shuffle(dbDocs.begin(), dbDocs.end(), rng);
        for (DbDoc dbDoc: dbDocs) {
            db2.push_back(std::pair {dbDoc, idRange});
        }
    }

    EncInd encInd1 = this->underlying.buildIndex(key1, db1);
    EncInd encInd2 = this->underlying.buildIndex(key2, db2);
    this->encInds = std::pair {encInd1, encInd2};
}

template <typename DbDoc, Underlying>
std::vector<IdOp> LogSrci<DbDoc, Underlying>::search(const KwRange& query) {
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
