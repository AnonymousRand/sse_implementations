#include <algorithm>
#include <random>

#include <openssl/rand.h>

#include "log_srci.h"
#include "pi_bas.h"
#include "util/openssl.h"

template <class DbDoc, class DbKw, template<class A, class B> class Underly>
LogSrci<DbDoc, DbKw, Underly>::LogSrci(
    Underly<SrciDb1Doc<DbKw>, DbKw>& underlying1, Underly<DbDoc, Id>& underlying2
) : underlying1(underlying1), underlying2(underlying2) {}

template <class DbDoc, class DbKw, template<class A, class B> class Underly>
void LogSrci<DbDoc, DbKw, Underly>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    // build index 1

    // build TDAG1 over keywords
    DbKw maxDbKw = -1;
    for (std::pair entry : db) {
        Range<DbKw> dbKwRange = entry.second;
        if (dbKwRange.second > maxDbKw) {
            maxDbKw = dbKwRange.second;
        }
    }
    this->tdag1 = TdagNode<DbKw>::buildTdag(maxDbKw);

    // figure out which documents share the same keywords by building index and list of unique kws like in PiBas
    std::unordered_map<Range<DbKw>, std::set<Id>> ind1;
    std::unordered_set<Range<DbKw>> uniqueDbKwRanges;
    for (std::pair entry : db) {
        DbDoc dbDoc = entry.first;
        Id id = dbDoc.getId();
        Range<DbKw> dbKwRange = entry.second;

        if (ind1.count(dbKwRange) == 0) {
            ind1[dbKwRange] = std::set {id};
        } else {
            ind1[dbKwRange].insert(id);
        }
        uniqueDbKwRanges.insert(dbKwRange);
    }

    // replicate every document (in this case pairs) to all keyword ranges/nodes in TDAG1 that cover it
    // thus `db1` contains the (inverted) pairs used to build index 1: ((kw, [ids]), kw range/node)
    Db<SrciDb1Doc<DbKw>, DbKw> db1;
    for (Range<DbKw> dbKwRange : uniqueDbKwRanges) {
        auto itIdsWithSameKw = ind1.find(dbKwRange);
        std::set<Id> idsWithSameKw = itIdsWithSameKw->second;
        IdRange idRange {*idsWithSameKw.begin(), *idsWithSameKw.rbegin()}; // this is why we used `set`
        SrciDb1Doc<DbKw> pair {dbKwRange, idRange};

        std::list<TdagNode<DbKw>*> ancestors = this->tdag1->getLeafAncestors(dbKwRange);
        for (TdagNode<DbKw>* ancestor : ancestors) {
            std::pair<SrciDb1Doc<DbKw>, Range<DbKw>> finalEntry {pair, ancestor->getRange()};
            db1.push_back(finalEntry);
        }
    }

    // build index 2

    // build TDAG2 over document ids
    Id maxId = Id(-1);
    for (std::pair entry : db) {
        DbDoc dbDoc = entry.first;
        Id id = dbDoc.getId();
        if (id > maxId) {
            maxId = id;
        }
    }
    this->tdag2 = TdagNode<Id>::buildTdag(maxId);

    // replicate every document to all id ranges/nodes in TDAG2 that cover it
    // again need temporary `unordered_map` index to shuffle
    std::unordered_map<IdRange, std::vector<DbDoc>> ind2;
    for (std::pair entry : db) {
        DbDoc dbDoc = entry.first;
        Id id = dbDoc.getId();
        std::list<TdagNode<Id>*> ancestors = this->tdag2->getLeafAncestors(Range<Id> {id, id});
        for (TdagNode<Id>* ancestor : ancestors) {
            Range<Id> ancestorIdRange = ancestor->getRange();
            if (ind2.count(ancestorIdRange) == 0) {
                ind2[ancestorIdRange] = std::vector {dbDoc};
            } else {
                ind2[ancestorIdRange].push_back(dbDoc);
            }
        }
    }

    // randomly permute dbDocuments associated with same id range/node and convert temporary `unordered_map` to `Db`
    Db<DbDoc, Id> db2;
    std::random_device rd;
    std::mt19937 rng(rd());
    for (std::pair pair : ind2) {
        IdRange idRange = pair.first;
        std::vector<DbDoc> dbDocs = pair.second;
        std::shuffle(dbDocs.begin(), dbDocs.end(), rng);
        for (DbDoc dbDoc: dbDocs) {
            db2.push_back(std::pair {dbDoc, idRange});
        }
    }

    this->underlying1.setup(secParam, db1);
    this->underlying2.setup(secParam, db2);
}

template <class DbDoc, class DbKw, template<class A, class B> class Underly>
std::vector<DbDoc> LogSrci<DbDoc, DbKw, Underly>::search(const Range<DbKw>& query) const {
    // query 1

    TdagNode<DbKw>* src1 = this->tdag1->findSrc(query);
    if (src1 == nullptr) { 
        return std::vector<DbDoc> {};
    }
    std::vector<SrciDb1Doc<DbKw>> choices = this->underlying1.search(src1->getRange());

    // query 2

    Id minId = Id(-1), maxId = Id(-1);
    // filter out unnecessary choices and merge remaining ones into a single id range
    for (SrciDb1Doc<DbKw> choice : choices) {
        Range<DbKw> choiceKwRange = choice.get().first;
        if (query.contains(choiceKwRange)) {
            IdRange choiceIdRange = choice.get().second;
            if (choiceIdRange.first < minId || minId == Id(-1)) {
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
        return std::vector<DbDoc> {};
    }
    return this->underlying2.search(src2->getRange());
}

template class LogSrci<Doc, Kw, PiBas>;
template class LogSrci<Id, Kw, PiBas>;
