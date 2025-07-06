#include "log_src_i.h"

#include "log_src_i_loc.cpp" // needed to bring in explicit template instatiation of `LogSrcILoc<PiBasLoc>`
#include "pi_bas.h"


/******************************************************************************/
/* `LogSrcIBase`                                                              */
/******************************************************************************/


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
LogSrcIBase<Underly>::LogSrcIBase() : underly1(new Underly<SrcIDb1Doc, Kw>()), underly2(new Underly<Doc, IdAlias>()) {}


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
LogSrcIBase<Underly>::~LogSrcIBase() {
    this->clear();
    if (this->underly1 != nullptr) {
        delete this->underly1;
        this->underly1 = nullptr;
    }
    if (this->underly2 != nullptr) {
        delete this->underly2;
        this->underly2 = nullptr;
    }
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
std::vector<Doc> LogSrcIBase<Underly>::search(const Range<Kw>& query, bool shouldCleanUpResults, bool isNaive) const {

    ///////////////////////////////// query 1 //////////////////////////////////

    Range<Kw> src1 = this->tdag1->findSrc(query);
    if (src1 == DUMMY_RANGE<Kw>()) { 
        return std::vector<Doc> {};
    }
    std::vector<SrcIDb1Doc> choices = this->underly1->search(src1, false, false);

    ///////////////////////////////// query 2 //////////////////////////////////

    // generate query for query 2 based on query 1 results
    // (filter out unnecessary choices and merge remaining ones into a single id range)
    IdAlias minIdAlias = DUMMY;
    IdAlias maxIdAlias = DUMMY;
    for (SrcIDb1Doc choice : choices) {
        Range<Kw> choiceKwRange = choice.get().first;
        if (!query.contains(choiceKwRange)) {
            continue;
        }
        Range<IdAlias> choiceIdAliasRange = choice.get().second;
        if (choiceIdAliasRange.first < minIdAlias || minIdAlias == DUMMY) {
            minIdAlias = choiceIdAliasRange.first;
        }
        if (choiceIdAliasRange.second > maxIdAlias || maxIdAlias == DUMMY) {
            maxIdAlias = choiceIdAliasRange.second;
        }
    }
    // if there are no choices or something went wrong
    if (minIdAlias == DUMMY || maxIdAlias == DUMMY) {
        return std::vector<Doc> {};
    }

    // perform query 2
    Range<IdAlias> query2 {minIdAlias, maxIdAlias};
    Range<IdAlias> src2 = this->tdag2->findSrc(query2);
    if (src2 == DUMMY_RANGE<IdAlias>()) {
        return std::vector<Doc> {};
    }
    return this->underly2->search(src2, shouldCleanUpResults, false);
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
void LogSrcIBase<Underly>::clear() {
    if (this->tdag1 != nullptr) {
        delete this->tdag1;
        this->tdag1 = nullptr;
    }
    if (this->tdag2 != nullptr) {
        delete this->tdag2;
        this->tdag2 = nullptr;
    }
    this->underly1->clear();
    this->underly2->clear();
    this->db.clear();
    this->_isEmpty = true;
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
Db<Doc, Kw> LogSrcIBase<Underly>::getDb() const {
    return this->db;
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
bool LogSrcIBase<Underly>::isEmpty() const {
    return this->_isEmpty;
}


template class LogSrcIBase<PiBas>;
template class LogSrcIBase<PiBasLoc>;


/******************************************************************************/
/* `LogSrcI`                                                                  */
/******************************************************************************/


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
LogSrcI<Underly>::LogSrcI() : LogSrcIBase<Underly>() {}


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
void LogSrcI<Underly>::setup(int secParam, const Db<Doc, Kw>& db) {
    this->clear();

    this->db = db;
    this->_isEmpty = db.empty();

    ////////////////////////////// build index 2 ///////////////////////////////

    // sort documents by keyword to assign index 2 nodes/"identifier aliases"
    auto sortByKw = [](const DbEntry<Doc, Kw>& dbEntry1, const DbEntry<Doc, Kw>& dbEntry2) {
        return dbEntry1.first.getKw() < dbEntry2.first.getKw();
    };

    Db<Doc, Kw> dbSorted = db;
    std::sort(dbSorted.begin(), dbSorted.end(), sortByKw);
    Db<SrcIDb1Doc, Kw> db1;
    Db<Doc, IdAlias> db2;
    db1.reserve(dbSorted.size());
    db2.reserve(dbSorted.size());
    for (long idAlias = 0; idAlias < dbSorted.size(); idAlias++) {
        DbEntry<Doc, Kw> dbEntry = dbSorted[idAlias];
        Doc doc = dbEntry.first;
        Range<Kw> kwRange = dbEntry.second;
        // populate `db1` leaves
        SrcIDb1Doc newDoc1 {kwRange, Range<IdAlias> {idAlias, idAlias}};
        DbEntry<SrcIDb1Doc, Kw> newDbEntry1 {newDoc1, kwRange};
        db1.push_back(newDbEntry1);
        // populate `db2` leaves
        DbEntry<Doc, IdAlias> newDbEntry2 = DbEntry {doc, Range<IdAlias> {idAlias, idAlias}};
        db2.push_back(newDbEntry2);
    }

    // build TDAG 2 over id aliases
    IdAlias maxIdAlias = 0;
    for (DbEntry<Doc, IdAlias> dbEntry : db2) {
        IdAlias idAlias = dbEntry.second.first;
        if (idAlias > maxIdAlias) {
            maxIdAlias = idAlias;
        }
    }
    this->tdag2 = new TdagNode<IdAlias>(Range<IdAlias> {0, maxIdAlias});

    // replicate every document to all id alias ranges/TDAG 2 nodes that cover it
    long stop = db2.size();
    long topLevel = std::log2(stop);
    long newSize = topLevel * (2 * stop) - (1 - std::pow(2, -topLevel)) * std::pow(2, topLevel+1) + stop;
    db2.reserve(newSize);
    for (long i = 0; i < stop; i++) {
        DbEntry<Doc, IdAlias> dbEntry = db2[i];
        Doc doc = dbEntry.first;
        Range<IdAlias> idAliasRange = dbEntry.second;
        std::list<Range<IdAlias>> ancestors = this->tdag2->getLeafAncestors(idAliasRange);
        for (Range<IdAlias> ancestor : ancestors) {
            // ancestors include the leaf itself, which is already in `db2`
            if (ancestor == idAliasRange) {
                continue;
            }
            db2.push_back(std::pair {doc, ancestor});
        }
    }

    this->underly2->setup(secParam, db2);

    ////////////////////////////// build index 1 ///////////////////////////////

    // build TDAG 1 over `Kw`s
    Range<Kw> kwBounds = findDbKwBounds(db1);
    this->tdag1 = new TdagNode<Kw>(kwBounds);

    // replicate every document (in this case `SrcIDb1Doc`s) to all keyword ranges/TDAG 1 nodes that cover it
    stop = db1.size();
    topLevel = std::log2(stop);
    newSize = topLevel * (2 * stop) - (1 - std::pow(2, -topLevel)) * std::pow(2, topLevel+1) + stop;
    db1.reserve(newSize);
    for (long i = 0; i < stop; i++) {
        DbEntry<SrcIDb1Doc, Kw> dbEntry = db1[i];
        SrcIDb1Doc doc = dbEntry.first;
        Range<Kw> kwRange = dbEntry.second;
        std::list<Range<Kw>> ancestors = this->tdag1->getLeafAncestors(kwRange);
        for (Range<Kw> ancestor : ancestors) {
            if (ancestor == kwRange) {
                continue;
            }
            db1.push_back(std::pair {doc, ancestor});
        }
    }

    this->underly1->setup(secParam, db1);
}


template class LogSrcI<PiBas>;
