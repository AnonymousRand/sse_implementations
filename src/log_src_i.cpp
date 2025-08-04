#include "log_src_i.h"

#include "log_src_i_loc.h"


/******************************************************************************/
/* `LogSrcIBase`                                                              */
/******************************************************************************/


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
LogSrcIBase<Underly>::LogSrcIBase() :
        underly1(new Underly<SrcIDb1Doc, Kw>()),
        underly2(new Underly<Doc<IdAlias>, IdAlias>()),
        origDbUnderly(new PiBas<Doc<>, Kw>()) {}


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
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
    if (this->origDbUnderly != nullptr) {
        delete this->origDbUnderly;
        this->origDbUnderly = nullptr;
    }
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
std::vector<Doc<>> LogSrcIBase<Underly>::search(const Range<Kw>& query, bool shouldCleanUpResults, bool isNaive) const {
    //--------------------------------------------------------------------------
    // query 1

    Range<Kw> src1 = this->tdag1->findSrc(query);
    if (src1 == DUMMY_RANGE<Kw>()) { 
        return std::vector<Doc<>> {};
    }
    std::vector<SrcIDb1Doc> choices = this->underly1->search(src1, false, false);

    //--------------------------------------------------------------------------
    // query 2

    // generate query for query 2 based on query 1 results
    // (filter out unnecessary choices and merge remaining ones into a single id range)
    IdAlias minIdAlias = DUMMY;
    IdAlias maxIdAlias = DUMMY;
    for (SrcIDb1Doc choice : choices) {
        Kw choiceKw = choice.get().first;
        if (!query.contains(choiceKw)) {
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
        return std::vector<Doc<>> {};
    }

    // perform query 2
    Range<IdAlias> query2 {minIdAlias, maxIdAlias};
    Range<IdAlias> src2 = this->tdag2->findSrc(query2);
    if (src2 == DUMMY_RANGE<IdAlias>()) {
        return std::vector<Doc<>> {};
    }
    return this->underly2->search(src2, shouldCleanUpResults, false);
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
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
    this->origDbUnderly->clear();
    this->size = 0;
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
void LogSrcIBase<Underly>::getDb(Db<Doc<>, Kw>& ret) const {
    this->origDbUnderly->getDb(ret);
}


template class LogSrcIBase<PiBas>;
template class LogSrcIBase<underly::PiBasLoc>;


/******************************************************************************/
/* `LogSrcI`                                                                  */
/******************************************************************************/


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
LogSrcI<Underly>::LogSrcI() : LogSrcIBase<Underly>() {}


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
void LogSrcI<Underly>::setup(int secParam, const Db<Doc<>, Kw>& db) {
    this->clear();

    this->secParam = secParam;
    this->size = db.size();
    this->origDbUnderly->setup(secParam, db);

    //--------------------------------------------------------------------------
    // build index 2

    // sort documents by keyword
    auto sortByKw = [](const DbEntry<Doc<>, Kw>& dbEntry1, const DbEntry<Doc<>, Kw>& dbEntry2) {
        return dbEntry1.first.getKw() < dbEntry2.first.getKw();
    };
    Db<Doc<>, Kw> dbSorted = db;
    std::sort(dbSorted.begin(), dbSorted.end(), sortByKw);

    // assign index 2 nodes/"identifier aliases" and populate both `db1` and `db2` leaves with this information
    Db<SrcIDb1Doc, Kw> db1;
    Db<Doc<IdAlias>, IdAlias> db2;
    db1.reserve(dbSorted.size());
    db2.reserve(dbSorted.size());
    Kw prevKw = DUMMY;
    IdAlias firstIdAliasWithKw;
    IdAlias lastIdAliasWithKw;
    auto addDb1Leaf = [&db1](Kw prevKw, IdAlias firstIdAliasWithKw, IdAlias lastIdAliasWithKw) {
        Range<IdAlias> idAliasRangeWithKw {firstIdAliasWithKw, lastIdAliasWithKw};
        Range<Kw> kwRange {prevKw, prevKw};
        SrcIDb1Doc newDoc {prevKw, idAliasRangeWithKw, kwRange};
        DbEntry<SrcIDb1Doc, Kw> newDbEntry {newDoc, kwRange};
        db1.push_back(newDbEntry);
    };
    for (long idAlias = 0; idAlias < dbSorted.size(); idAlias++) {
        DbEntry<Doc<>, Kw> dbEntry = dbSorted[idAlias];
        Doc<> doc = dbEntry.first;
        Kw kw = dbEntry.second.first; // entries in `db` must have size 1 `Kw` ranges!
        // populate `db2` leaves
        Range<IdAlias> idAliasRange {idAlias, idAlias};
        Doc<IdAlias> newDb2Doc(doc.get(), idAliasRange);
        DbEntry<Doc<IdAlias>, IdAlias> newDb2Entry = DbEntry {newDb2Doc, idAliasRange};
        db2.push_back(newDb2Entry);

        // populate `db1` leaves
        if (kw != prevKw) {
            if (prevKw != DUMMY) {
                addDb1Leaf(prevKw, firstIdAliasWithKw, lastIdAliasWithKw);
            }
            prevKw = kw;
            firstIdAliasWithKw = idAlias;
            lastIdAliasWithKw = idAlias;
        } else {
            lastIdAliasWithKw = idAlias;
        }
    }
    // make sure to write in last `Kw` (which cannot be detected by `kw != prevKw` in the loop above)
    if (prevKw != DUMMY) {
        addDb1Leaf(prevKw, firstIdAliasWithKw, lastIdAliasWithKw);
    }

    // build TDAG 2 over id aliases
    IdAlias maxIdAlias = 0;
    for (DbEntry<Doc<IdAlias>, IdAlias> dbEntry : db2) {
        IdAlias idAlias = dbEntry.second.first;
        if (idAlias > maxIdAlias) {
            maxIdAlias = idAlias;
        }
    }
    this->tdag2 = new TdagNode<IdAlias>(Range<IdAlias> {0, maxIdAlias});

    // replicate every document to all id alias ranges/TDAG 2 nodes that cover it
    long stop = db2.size();
    db2.reserve(calcTdagItemCount(stop));
    for (long i = 0; i < stop; i++) {
        DbEntry<Doc<IdAlias>, IdAlias> dbEntry = db2[i];
        Doc<IdAlias> doc = dbEntry.first;
        Range<IdAlias> idAliasRange = dbEntry.second;
        std::list<Range<IdAlias>> ancestors = this->tdag2->getLeafAncestors(idAliasRange);
        for (Range<IdAlias> ancestor : ancestors) {
            // ancestors include the leaf itself, which is already in `db2`
            if (ancestor == idAliasRange) {
                continue;
            }
            Doc<IdAlias> newDoc(doc.get(), ancestor);
            db2.push_back(std::pair {newDoc, ancestor});
        }
    }

    this->underly2->setup(secParam, db2);

    //--------------------------------------------------------------------------
    // build index 1

    // build TDAG 1 over `Kw`s
    Range<Kw> db1KwBounds = findDbKwBounds(db1);
    this->tdag1 = new TdagNode<Kw>(db1KwBounds);

    // replicate every document (in this case `SrcIDb1Doc`s) to all keyword ranges/TDAG 1 nodes that cover it
    stop = db1.size();
    db1.reserve(calcTdagItemCount(stop));
    for (long i = 0; i < stop; i++) {
        DbEntry<SrcIDb1Doc, Kw> dbEntry = db1[i];
        SrcIDb1Doc doc = dbEntry.first;
        Range<Kw> kwRange = dbEntry.second;
        std::list<Range<Kw>> ancestors = this->tdag1->getLeafAncestors(kwRange);
        for (Range<Kw> ancestor : ancestors) {
            if (ancestor == kwRange) {
                continue;
            }
            SrcIDb1Doc newDoc(doc.get(), ancestor);
            db1.push_back(std::pair {newDoc, ancestor});
        }
    }

    this->underly1->setup(secParam, db1);
}


template class LogSrcI<PiBas>;
