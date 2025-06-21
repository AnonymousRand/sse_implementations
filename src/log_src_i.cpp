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

    // build TDAG 2 over id aliases
    IdAlias maxIdAlias = IdAlias(0);
    for (DbEntry<DbDoc, IdAlias> dbEntry : db2) {
        IdAlias idAlias = dbEntry.second.first;
        if (idAlias > maxIdAlias) {
            maxIdAlias = idAlias;
        }
    }
    this->tdag2 = new TdagNode<IdAlias>(maxIdAlias);

    // replicate every document to all id alias ranges/TDAG 2 nodes that cover it
    int stop = db2.size();
    for (int i = 0; i < stop; i++) {
        DbEntry<DbDoc, IdAlias> dbEntry = db2[i];
        DbDoc dbDoc = dbEntry.first;
        Range<IdAlias> idAliasRange = dbEntry.second;
        std::list<Range<IdAlias>> ancestors = this->tdag2->getLeafAncestors(idAliasRange);
        for (Range<IdAlias> ancestor : ancestors) {
            // ancestors include the leaf itself, which is already in `db2`
            if (ancestor == idAliasRange) {
                continue;
            }
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

    // replicate every document (in this case `SrcIDb1Doc`s) to all keyword ranges/TDAG 1 nodes that cover it
    stop = db1.size();
    for (int i = 0; i < stop; i++) {
        DbEntry<SrcIDb1Doc<DbKw>, DbKw> dbEntry = db1[i];
        SrcIDb1Doc<DbKw> dbDoc = dbEntry.first;
        Range<DbKw> dbKwRange = dbEntry.second;
        std::list<Range<DbKw>> ancestors = this->tdag1->getLeafAncestors(dbKwRange);
        for (Range<DbKw> ancestor : ancestors) {
            if (ancestor == dbKwRange) {
                continue;
            }
            db1.push_back(std::pair {dbDoc, ancestor});
        }
    }

    this->underly1.setup(secParam, db1);
    this->underly2.setup(secParam, db2);
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
Range<IdAlias> LogSrcI<Underly, DbDoc, DbKw>::searchBase(const Range<DbKw>& query) const {
    // query 1

    Range<DbKw> src1 = this->tdag1->findSrc(query);
    if (src1 == DUMMY_RANGE<DbKw>()) { 
        return DUMMY_RANGE<Id>();
    }
    std::vector<SrcIDb1Doc<DbKw>> choices = this->underly1.search(src1);

    // query 2

    IdAlias minIdAlias = DUMMY;
    IdAlias maxIdAlias = DUMMY;
    // filter out unnecessary choices and merge remaining ones into a single id range
    for (SrcIDb1Doc<DbKw> choice : choices) {
        Range<DbKw> choiceKwRange = choice.get().first;
        if (query.contains(choiceKwRange)) {
            Range<IdAlias> choiceIdAliasRange = choice.get().second;
            if (choiceIdAliasRange.first < minIdAlias || minIdAlias == DUMMY) {
                minIdAlias = choiceIdAliasRange.first;
            }
            if (choiceIdAliasRange.second > maxIdAlias || maxIdAlias == DUMMY) {
                maxIdAlias = choiceIdAliasRange.second;
            }
        }
    }

    Range<IdAlias> idAliasRangeToQuery {minIdAlias, maxIdAlias};
    Range<IdAlias> src2 = this->tdag2->findSrc(idAliasRangeToQuery);
    return src2;
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
std::vector<DbDoc> LogSrcI<Underly, DbDoc, DbKw>::search(const Range<DbKw>& query) const {
    Range<IdAlias> src2 = this->searchBase(query);
    if (src2 == DUMMY_RANGE<IdAlias>()) {
        return std::vector<DbDoc> {};
    }
    return this->underly2.search(src2);
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
std::vector<DbDoc> LogSrcI<Underly, DbDoc, DbKw>::searchWithoutHandlingDels(const Range<DbKw>& query) const {
    Range<IdAlias> src2 = this->searchBase(query);
    if (src2 == DUMMY_RANGE<IdAlias>()) {
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
