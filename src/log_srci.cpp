#include <algorithm>
#include <random>

#include <openssl/rand.h>

#include "log_srci.h"
#include "util/openssl.h"

////////////////////////////////////////////////////////////////////////////////
// Client
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
LogSrciClient<Underlying>::LogSrciClient(Underlying underlying)
        : IRangeSseClient<std::pair<ustring, ustring>, std::pair<EncInd, EncInd>, Underlying>(underlying) {}

template <typename Underlying>
std::pair<ustring, ustring> LogSrciClient<Underlying>::setup(int secParam) {
    unsigned char* key1 = new unsigned char[secParam];
    unsigned char* key2 = new unsigned char[secParam];
    int res1 = RAND_priv_bytes(key1, secParam);
    int res2 = RAND_priv_bytes(key2, secParam);
    if (res1 != 1 || res2 != 1) {
        handleOpenSslErrors();
    }

    ustring ustrKey1 = toUstr(key1, secParam);
    ustring ustrKey2 = toUstr(key2, secParam);
    delete[] key1, key2;
    return std::pair<ustring, ustring> {ustrKey1, ustrKey2};
}

template <typename Underlying>
std::pair<EncInd, EncInd> LogSrciClient<Underlying>::buildIndex(std::pair<ustring, ustring> key, Db<> db) {
    ustring key1 = key.first;
    ustring key2 = key.second;

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
    std::map<KwRange, std::set<Id>> index;
    std::set<KwRange> uniqueKwRanges;
    for (auto entry : db) {
        Id id = std::get<0>(entry);
        KwRange kwRange = std::get<1>(entry);

        if (index.count(kwRange) == 0) {
            index[kwRange] = std::set<Id> {id};
        } else {
            index[kwRange].insert(id);
        }
        uniqueKwRanges.insert(kwRange);
    }

    // replicate every document (in this case pairs) to all keyword ranges/nodes in TDAG1 that cover it
    // thus `db1` contains the (inverted) pairs used to build index 1: ((kw, id range), kw range/node)
    Db<SrciDb1Doc, KwRange> db1;
    for (KwRange kwRange : uniqueKwRanges) {
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
    // again need temporary `map` to shuffle
    std::map<IdRange, std::vector<Id>> tempInd2;
    for (auto entry : db) {
        Id id = std::get<0>(entry);
        std::list<TdagNode<Id>*> ancestors = this->tdag2->getLeafAncestors(IdRange {id, id});
        for (TdagNode<Id>* ancestor : ancestors) {
            IdRange ancestorIdRange = ancestor->getRange();
            if (tempInd2.count(ancestorIdRange) == 0) {
                tempInd2[ancestorIdRange] = std::vector<Id> {id};
            } else {
                tempInd2[ancestorIdRange].push_back(id);
            }
        }
    }

    // randomly permute documents associated with same id range/node and convert temporary `map` to `Db`
    Db<Id, IdRange> db2;
    std::random_device rd;
    std::mt19937 rng(rd());
    for (auto pair : tempInd2) {
        IdRange idRange = pair.first;
        std::vector<Id> ids = pair.second;
        std::shuffle(ids.begin(), ids.end(), rng);
        for (Id id: ids) {
            db2.push_back(std::pair <Id, IdRange> {id, idRange});
        }
    }

    EncInd encInd1 = this->underlying.buildIndexGeneric(key1, db1);
    EncInd encInd2 = this->underlying.buildIndexGeneric(key2, db2);
    return std::pair<EncInd, EncInd> {encInd1, encInd2};
}

template <typename Underlying>
QueryToken LogSrciClient<Underlying>::trpdr(ustring key1, KwRange kwRange) {
    TdagNode<Kw>* src = this->tdag1->findSrc(kwRange);
    return this->underlying.trpdrGeneric(key1, src->getRange());
}

template <typename Underlying>
QueryToken LogSrciClient<Underlying>::trpdr2(ustring key2, KwRange kwRange, std::vector<SrciDb1Doc> choices) {
    Id minId = -1, maxId = -1;
    // filter out unnecessary choices and merge remaining ones into a single id range
    for (SrciDb1Doc choice : choices) {
        KwRange choiceKwRange = choice.get().first;
        if (kwRange.contains(choiceKwRange)) {
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
    TdagNode<Id>* src = this->tdag2->findSrc(idRangeToQuery);
    return this->underlying.trpdrGeneric(key2, src->getRange());
}

template class LogSrciClient<PiBasClient>;

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
LogSrciServer<Underlying>::LogSrciServer(Underlying underlying) : IRangeSseServer<Underlying>(underlying) {}

template <typename Underlying>
std::vector<SrciDb1Doc> LogSrciServer<Underlying>::search1(EncInd encInd1, QueryToken queryToken) {
    return this->underlying.template searchGeneric<SrciDb1Doc>(encInd1, queryToken);
}

template <typename Underlying>
std::vector<Id> LogSrciServer<Underlying>::search(EncInd encInd2, QueryToken queryToken) {
    return this->underlying.template searchGeneric<Id>(encInd2, queryToken);
}

template class LogSrciServer<PiBasServer>;
