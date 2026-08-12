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


template <template <class ...> class Underly> requires IsSse<Underly<Record<>, Kw>>
void LogSrcI<Underly>::setup(int secParam, const Db<Record<>, Kw>& db) {
    this->clear();

    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();

    //--------------------------------------------------------------------------
    // build index 2

    // sort documents by keyword
    auto sortByKw = [](const DbRecord<Record<>, Kw>& dbRecord1, const DbRecord<Record<>, Kw>& dbRecord2) {
        return dbRecord1.first.getKw() < dbRecord2.first.getKw();
    };
    Db<Record<>, Kw> dbSorted = db;
    std::sort(dbSorted.begin(), dbSorted.end(), sortByKw);

    // assign index 2 nodes/"identifier aliases" and populate both `db1` and `db2`
    // leaves with this information
    Db<SrcIDb1Record, Kw> db1;
    Db<Record<IdAlias>, IdAlias> db2;
    db1.reserve(dbSorted.size());
    db2.reserve(dbSorted.size());
    Kw prevKw = DUMMY;
    IdAlias firstIdAliasWithKw;
    IdAlias lastIdAliasWithKw;
    auto addDb1Leaf = [&db1](Kw prevKw, IdAlias firstIdAliasWithKw, IdAlias lastIdAliasWithKw) {
        Range<IdAlias> idAliasRangeWithKw {firstIdAliasWithKw, lastIdAliasWithKw};
        Range<Kw> kwRange {prevKw, prevKw};
        SrcIDb1Record newRecord {prevKw, idAliasRangeWithKw, kwRange};
        DbRecord<SrcIDb1Record, Kw> newDbRecord {newRecord, kwRange};
        db1.push_back(newDbRecord);
    };
    for (int64_t idAlias = 0; idAlias < dbSorted.size(); idAlias++) {
        DbRecord<Record<>, Kw> dbRecord = dbSorted[idAlias];
        Record<> record = dbRecord.first;
        Kw kw = dbRecord.second.first; // records in `db` must have size 1 `Kw` ranges!
        // populate `db2` leaves
        Range<IdAlias> idAliasRange {idAlias, idAlias};
        Record<IdAlias> newDb2Record(record.get(), idAliasRange);
        DbRecord<Record<IdAlias>, IdAlias> newDb2Record = DbRecord {newDb2Record, idAliasRange};
        db2.push_back(newDb2Record);

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
    for (DbRecord<Record<IdAlias>, IdAlias> dbRecord : db2) {
        IdAlias idAlias = dbRecord.second.first;
        if (idAlias > maxIdAlias) {
            maxIdAlias = idAlias;
        }
    }
    this->tdag2 = new TdagNode<IdAlias>(Range<IdAlias> {0, maxIdAlias});

    // replicate every document to all id alias ranges/TDAG 2 nodes that cover it
    int64_t db2Size = db2.size();
    db2.reserve(utils::calcTdagRecordCount(db2Size));
    for (int64_t i = 0; i < db2Size; i++) {
        DbRecord<Record<IdAlias>, IdAlias> dbRecord = db2[i];
        Record<IdAlias> record = dbRecord.first;
        Range<IdAlias> idAliasRange = dbRecord.second;
        std::list<Range<IdAlias>> ancestors = this->tdag2->getLeafAncestors(idAliasRange);
        for (Range<IdAlias> ancestor : ancestors) {
            // ancestors include the leaf itself, which is already in `db2`
            if (ancestor == idAliasRange) {
                continue;
            }
            Record<IdAlias> newRecord(record.get(), ancestor);
            db2.push_back(std::pair {newRecord, ancestor});
        }
    }

    this->underly2->setup(secParam, db2);

    //--------------------------------------------------------------------------
    // build index 1

    // build TDAG 1 over `Kw`s
    Range<Kw> db1KwBounds = utils::findDbKwBounds(db1);
    this->tdag1 = new TdagNode<Kw>(db1KwBounds);

    // replicate every document (in this case `SrcIDb1Record`s) to all keyword ranges/
    // TDAG 1 nodes that cover it
    int64_t db1Size = db1.size();
    db1.reserve(utils::calcTdagRecordCount(db1Size));
    for (int64_t i = 0; i < db1Size; i++) {
        DbRecord<SrcIDb1Record, Kw> dbRecord = db1[i];
        SrcIDb1Record record = dbRecord.first;
        Range<Kw> kwRange = dbRecord.second;
        std::list<Range<Kw>> ancestors = this->tdag1->getLeafAncestors(kwRange);
        for (Range<Kw> ancestor : ancestors) {
            if (ancestor == kwRange) {
                continue;
            }
            SrcIDb1Record newRecord(record.get(), ancestor);
            db1.push_back(std::pair {newRecord, ancestor});
        }
    }

    this->underly1->setup(secParam, db1);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class LogSrcI<PiBas>;
template class LogSrcI<NLogN>;
