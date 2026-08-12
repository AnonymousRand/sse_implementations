#include "schemes/log_src_i/log_src_i.h"

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <list>
#include <utility>

#include "schemes/interfaces/sse.h"
#include "schemes/log_src_i/log_src_i_base.h"

// for explicit template instantiation
#include "schemes/n_log_n/n_log_n.h"
#include "schemes/pi_bas/pi_bas.h"

#include "utils/db.h"
#include "utils/range.h"
#include "utils/sse_utils.h"
#include "utils/tdag.h"


//------------------------------------------------------------------------------
// `ISse`


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
void LogSrcI<Underly>::setup(int secParam, const Db<Doc<>, Kw>& db) {
    this->clear();

    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();

    //--------------------------------------------------------------------------
    // build index 2

    // sort documents by keyword
    auto sortByKw = [](const DbEntry<Doc<>, Kw>& dbEntry1, const DbEntry<Doc<>, Kw>& dbEntry2) {
        return dbEntry1.first.getKw() < dbEntry2.first.getKw();
    };
    Db<Doc<>, Kw> dbSorted = db;
    std::sort(dbSorted.begin(), dbSorted.end(), sortByKw);

    // assign index 2 nodes/"identifier aliases" and populate both `db1` and `db2`
    // leaves with this information
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
    for (int64_t idAlias = 0; idAlias < dbSorted.size(); idAlias++) {
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
    // make sure to write in last `Kw` (which cannot be detected by `kw != prevKw`
    // in the loop above)
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
    int64_t db2Size = db2.size();
    db2.reserve(utils::calcTdagEntryCount(db2Size));
    for (int64_t i = 0; i < db2Size; i++) {
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
    Range<Kw> db1KwBounds = utils::findDbKwBounds(db1);
    this->tdag1 = new TdagNode<Kw>(db1KwBounds);

    // replicate every document (in this case `SrcIDb1Doc`s) to all keyword ranges/
    // TDAG 1 nodes that cover it
    int64_t db1Size = db1.size();
    db1.reserve(utils::calcTdagEntryCount(db1Size));
    for (int64_t i = 0; i < db1Size; i++) {
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


//------------------------------------------------------------------------------
// explicit template instantiations


template class LogSrcI<PiBas>;
template class LogSrcI<NLogN>;
