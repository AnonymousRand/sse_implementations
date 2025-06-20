#include <algorithm>

#include <openssl/rand.h>

#include "log_src_i.h"
#include "pi_bas.h"
#include "util/openssl.h"

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
LogSrcI<Underly, DbDoc, DbKw>::LogSrcI(
    const Underly<SrcIDb1Doc<DbKw>, DbKw>& underly1, const Underly<DbDoc, Id>& underly2
) : underly1(underly1), underly2(underly2) {}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
LogSrcI<Underly, DbDoc, DbKw>::LogSrcI() : LogSrcI(Underly<SrcIDb1Doc<DbKw>, DbKw>(), Underly<DbDoc, Id>()) {}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
LogSrcI<Underly, DbDoc, DbKw>::~LogSrcI() {
    if (this->tdag1 != nullptr) {
        delete this->tdag1;
    }
    if (this->tdag2 != nullptr) {
        delete this->tdag2;
    }
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
void LogSrcI<Underly, DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->db = db;
    this->_isEmpty = this->db.empty();

    std::cout << "base db" << std::endl;
    for (auto r : db) {
        std::cout << r.first << "," << r.second << std::endl;
    }

    // build index 2

    // sort documents by keyword to assign index 2 nodes/"identifier aliases"
    auto sortByKw = [](const DbEntry<DbDoc, DbKw>& dbEntry1, const DbEntry<DbDoc, DbKw>& dbEntry2) {
        return dbEntry1.second <= dbEntry2.second;
    };

    Db<DbDoc, DbKw> dbSorted = db;
    std::sort(dbSorted.begin(), dbSorted.end(), sortByKw);
    Db<DbDoc, IdAlias> db2;
    std::unordered_map<Id, IdAlias> idAliasMapping; // for quick reference when buiding index 1
    for (int i = 0; i < dbSorted.size(); i++) {
        DbEntry<DbDoc, DbKw> dbEntry = dbSorted[i];
        DbEntry<DbDoc, IdAlias> newDbEntry = DbEntry {dbEntry.first, Range<IdAlias> {i, i}};
        db2.push_back(newDbEntry);
        Id id = dbEntry.first.getId();
        idAliasMapping[id] = IdAlias(i);
    }
    std::cout << "db2 before replications:" << std::endl;
    for (auto r : db2) {
        std::cout << r.first << "," << r.second << std::endl;
    }

    // build TDAG 2 over id aliases
    IdAlias maxIdAlias = IdAlias(0);
    for (DbEntry<DbDoc, IdAlias> dbEntry : db2) {
        IdAlias idAlias = dbEntry.second.first;
        if (idAlias > maxIdAlias) {
            maxIdAlias = idAlias;
        }
    }
    this->tdag2 = new TdagNode<IdAlias>(maxIdAlias);

    // replicate every document to all id alias ranges/nodes in TDAG 2 that cover it
    // TODO: make sure no Range<Id> anymore or TdagNode<Id>
    int stop = db2.size();
    for (int i = 0; i < stop; i++) {
        DbEntry<DbDoc, IdAlias> dbEntry = db2[i];
        DbDoc dbDoc = dbEntry.first;
        IdAlias idAlias = dbEntry.second.first;
        std::list<Range<IdAlias>> ancestors = this->tdag2->getLeafAncestors(Range<IdAlias> {idAlias, idAlias});
        for (Range<IdAlias> ancestor : ancestors) {
            db2.push_back(std::pair {dbDoc, ancestor});
        }
    }

    // build index 1

    // build TDAG 1 over keywords
    DbKw maxDbKw = findMaxDbKw(db);
    this->tdag1 = new TdagNode<DbKw>(maxDbKw);

    // assign id aliases/TDAG 2 nodes to documents based on index 2
    Db<SrcIDb1Doc<DbKw>, DbKw> db1;
    for (DbEntry<DbDoc, DbKw> dbEntry : db) {
        DbDoc dbDoc = dbEntry.first;
        Range<DbKw> dbKwRange = dbEntry.second;
        IdAlias idAlias = idAliasMapping[dbDoc.getId()];
        // TODO can all Range inisitalizations drop the template arg?
        SrcIDb1Doc<DbKw> pair {dbKwRange, Range<IdAlias> {idAlias, idAlias}};
        DbEntry<SrcIDb1Doc<DbKw>, DbKw> newDbEntry = std::pair {pair, dbKwRange}; // `std::pair` works, not `DbEntry`
        db1.push_back(newDbEntry);
    }
    std::cout << "db1 before replications:" << std::endl;
    for (auto r : db1) {
        std::cout << r.first << "," << r.second << std::endl;
    }

    // TODO finish; also what is this part for??
    // figure out which documents share the same keywords by building index and list of unique kws like in PiBas
    //Ind<DbKw, Id> ind1;
    //for (DbEntry<DbDoc, DbKw> dbEntry : db) {
    //    DbDoc dbDoc = dbEntry.first;
    //    Id id = dbDoc.getId();
    //    Range<DbKw> dbKwRange = dbEntry.second;

    //    if (ind1.count(dbKwRange) == 0) {
    //        ind1[dbKwRange] = std::vector {id};
    //    } else {
    //        ind1[dbKwRange].push_back(id);
    //    }
    //}

    // replicate every document (in this case pairs) to all keyword ranges/nodes in TDAG 1 that cover it
    // thus `db1` contains the (inverted) pairs used to build index 1: ((kw, [ids]), kw range/node)
    //std::unordered_set<Range<DbKw>> uniqDbKwRanges = getUniqDbKwRanges(db);
    //for (Range<DbKw> dbKwRange : uniqDbKwRanges) {
    //    auto itIdsWithSameKw = ind1.find(dbKwRange);
    //    std::vector<Id> idsWithSameKw = itIdsWithSameKw->second;
    //    Id minId = Id(0), maxId = Id(0);
    //    for (Id id : idsWithSameKw) {
    //        if (id > maxId) {
    //            maxId = id;
    //        }
    //        if (id < minId || minId == Id(0)) {
    //            minId = id;
    //        }
    //    }
    //    Range<Id> idRange {minId, maxId}; // this is why we used `set`
    //    SrcIDb1Doc<DbKw> pair {dbKwRange, idRange};

    //    std::list<Range<DbKw>> ancestors = this->tdag1->getLeafAncestors(dbKwRange);
    //    for (Range<DbKw> ancestor : ancestors) {
    //        std::pair<SrcIDb1Doc<DbKw>, Range<DbKw>> finalEntry {pair, ancestor};
    //        db1.push_back(finalEntry);
    //    }
    //}

    //this->underly1.setup(secParam, db1);
    this->underly2.setup(secParam, db2);
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
Range<Id> LogSrcI<Underly, DbDoc, DbKw>::searchBase(const Range<DbKw>& query) const {
    // query 1

    Range<DbKw> src1 = this->tdag1->findSrc(query);
    if (src1 == DUMMY_RANGE<DbKw>()) { 
        return DUMMY_RANGE<Id>();
    }
    std::vector<SrcIDb1Doc<DbKw>> choices = this->underly1.search(src1);

    // query 2

    Id minId = DUMMY;
    Id maxId = DUMMY;
    // filter out unnecessary choices and merge remaining ones into a single id range
    for (SrcIDb1Doc<DbKw> choice : choices) {
        Range<DbKw> choiceKwRange = choice.get().first;
        if (query.contains(choiceKwRange)) {
            Range<Id> choiceIdRange = choice.get().second;
            if (choiceIdRange.first < minId || minId == DUMMY) {
                minId = choiceIdRange.first;
            }
            if (choiceIdRange.second > maxId || maxId == DUMMY) {
                maxId = choiceIdRange.second;
            }
        }
    }

    Range<Id> idRangeToQuery {minId, maxId};
    Range<Id> src2 = this->tdag2->findSrc(idRangeToQuery);
    return src2;
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
std::vector<DbDoc> LogSrcI<Underly, DbDoc, DbKw>::search(const Range<DbKw>& query) const {
    Range<Id> src2 = this->searchBase(query);
    if (src2 == DUMMY_RANGE<Id>()) {
        return std::vector<DbDoc> {};
    }
    return this->underly2.search(src2);
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
std::vector<DbDoc> LogSrcI<Underly, DbDoc, DbKw>::searchWithoutHandlingDels(const Range<DbKw>& query) const {
    Range<Id> src2 = this->searchBase(query);
    if (src2 == DUMMY_RANGE<Id>()) {
        return std::vector<DbDoc> {};
    }
    return this->underly2.searchWithoutHandlingDels(src2);
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
Db<DbDoc, DbKw> LogSrcI<Underly, DbDoc, DbKw>::getDb() const {
    return this->db;
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
bool LogSrcI<Underly, DbDoc, DbKw>::isEmpty() const {
    return this->_isEmpty;
}

template class LogSrcI<PiBas, Id, Kw>;
template class LogSrcI<PiBas, IdOp, Kw>;

template class LogSrcI<PiBasResHiding, Id, Kw>;
template class LogSrcI<PiBasResHiding, IdOp, Kw>;
