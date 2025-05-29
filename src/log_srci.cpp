#include <openssl/rand.h>

#include "log_srci.h"
#include "pi_bas.h"
#include "util/openssl.h"

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
LogSrci<Underly, DbDoc, DbKw>::LogSrci() : LogSrci(Underly<SrciDb1Doc<DbKw>, DbKw>(), Underly<DbDoc, Id>()) {}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
LogSrci<Underly, DbDoc, DbKw>::LogSrci(
    const Underly<SrciDb1Doc<DbKw>, DbKw>& underly1, const Underly<DbDoc, Id>& underly2
) : underly1(underly1), underly2(underly2) {}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
void LogSrci<Underly, DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->db = db;
    this->_isEmpty = this->db.empty();

    // build index 1

    // build TDAG1 over keywords
    DbKw maxDbKw = findMaxDbKw(db);
    this->tdag1 = new TdagNode<DbKw>(maxDbKw);

    // figure out which documents share the same keywords by building index and list of unique kws like in PiBas
    Ind<DbKw, Id> ind1;
    for (DbEntry<DbDoc, DbKw> entry : db) {
        DbDoc dbDoc = entry.first;
        Id id = dbDoc.getId();
        Range<DbKw> dbKwRange = entry.second;

        if (ind1.count(dbKwRange) == 0) {
            ind1[dbKwRange] = std::vector {id};
        } else {
            ind1[dbKwRange].push_back(id);
        }
    }
    std::unordered_set<Range<DbKw>> uniqDbKwRanges = getUniqDbKwRanges(db);

    // replicate every document (in this case pairs) to all keyword ranges/nodes in TDAG1 that cover it
    // thus `db1` contains the (inverted) pairs used to build index 1: ((kw, [ids]), kw range/node)
    Db<SrciDb1Doc<DbKw>, DbKw> db1;
    for (Range<DbKw> dbKwRange : uniqDbKwRanges) {
        auto itIdsWithSameKw = ind1.find(dbKwRange);
        std::vector<Id> idsWithSameKw = itIdsWithSameKw->second;
        Id minId = Id(0), maxId = Id(0);
        for (Id id : idsWithSameKw) {
            if (id > maxId) {
                maxId = id;
            }
            if (id < minId || minId == Id(0)) {
                minId = id;
            }
        }
        IdRange idRange {minId, maxId}; // this is why we used `set`
        SrciDb1Doc<DbKw> pair {dbKwRange, idRange};

        std::list<TdagNode<DbKw>*> ancestors = this->tdag1->getLeafAncestors(dbKwRange);
        for (TdagNode<DbKw>* ancestor : ancestors) {
            std::pair<SrciDb1Doc<DbKw>, Range<DbKw>> finalEntry {pair, ancestor->getRange()};
            db1.push_back(finalEntry);
        }
    }

    // build index 2

    // build TDAG2 over document ids
    // PRECONDITION: document ids are positive
    Id maxId = Id(0);
    for (DbEntry<DbDoc, DbKw> entry : db) {
        DbDoc dbDoc = entry.first;
        Id id = dbDoc.getId();
        if (id > maxId) {
            maxId = id;
        }
    }
    this->tdag2 = new TdagNode<Id>(maxId);

    // replicate every document to all id ranges/nodes in TDAG2 that cover it
    // again need temporary `unordered_map` index to shuffle
    Ind<Id, DbDoc> ind2;
    for (DbEntry<DbDoc, DbKw> entry : db) {
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

    // randomly permute documents associated with same id range/node and convert temporary `unordered_map` to `Db`
    shuffleInd(ind2);
    Db<DbDoc, Id> db2 = indToDb(ind2);

    this->underly1.setup(secParam, db1);
    this->underly2.setup(secParam, db2);
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
Range<Id> LogSrci<Underly, DbDoc, DbKw>::searchBase(const Range<DbKw>& query) const {
    // query 1

    Range<DbKw> src1 = this->tdag1->findSrc(query);
    if (src1 == DUMMY_RANGE<DbKw>()) { 
        return DUMMY_RANGE<Id>();
    }
    std::vector<SrciDb1Doc<DbKw>> choices = this->underly1.search(src1);

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
    Range<Id> src2 = this->tdag2->findSrc(idRangeToQuery);
    return src2;
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
std::vector<DbDoc> LogSrci<Underly, DbDoc, DbKw>::search(const Range<DbKw>& query) const {
    Range<Id> src2 = this->searchBase(query);
    if (src2 == DUMMY_RANGE<Id>()) {
        return std::vector<DbDoc> {};
    }
    return this->underly2.search(src2);
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
std::vector<DbDoc> LogSrci<Underly, DbDoc, DbKw>::searchWithoutHandlingDels(const Range<DbKw>& query) const {
    Range<Id> src2 = this->searchBase(query);
    if (src2 == DUMMY_RANGE<Id>()) {
        return std::vector<DbDoc> {};
    }
    return this->underly2.searchWithoutHandlingDels(src2);
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
Db<DbDoc, DbKw> LogSrci<Underly, DbDoc, DbKw>::getDb() const {
    return this->db;
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
bool LogSrci<Underly, DbDoc, DbKw>::isEmpty() const {
    return this->_isEmpty;
}

template class LogSrci<PiBas, Id, Kw>;
template class LogSrci<PiBas, IdOp, Kw>;

template class LogSrci<PiBasResHiding, Id, Kw>;
template class LogSrci<PiBasResHiding, IdOp, Kw>;
