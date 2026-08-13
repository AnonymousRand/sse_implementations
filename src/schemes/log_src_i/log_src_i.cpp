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
#include "utils/types.h"


//------------------------------------------------------------------------------
// `ISse`


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>, Kw>>
void LogSrcI<Underly>::setup(int secParam, const Db<Tuple<>>& db) {
    this->clear();

    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();

    //--------------------------------------------------------------------------
    // build index 2

    // sort documents by keyword
    Db<Tuple<>> dbSorted = db;
    auto sortByKw = [](const Tuple<>& tuple1, const Tuple<>& tuple2) {
        return tuple1.getKw() < tuple2.getKw();
    };
    std::sort(dbSorted.begin(), dbSorted.end(), sortByKw);

    // assign index 2 nodes/"identifier aliases" and populate both `db1` and `db2`
    // leaves with this information
    Db<SrcIDb1Tuple> db1;
    Db<Tuple<IdAlias>> db2;
    db1.reserve(dbSorted.size());
    db2.reserve(dbSorted.size());
    Kw prevKw = DUMMY;
    IdAlias firstIdAliasWithKw;
    IdAlias lastIdAliasWithKw;
    auto addDb1Leaf = [&db1](Kw prevKw, IdAlias firstIdAliasWithKw, IdAlias lastIdAliasWithKw) {
        Range<IdAlias> idAliasRangeWithKw {firstIdAliasWithKw, lastIdAliasWithKw};
        Range<Kw> kwRange {prevKw, prevKw};
        SrcIDb1Tuple newTuple {prevKw, idAliasRangeWithKw, kwRange};
        db1.push_back(newTuple);
    };

    for (int64_t idAlias = 0; idAlias < dbSorted.size(); idAlias++) {
        Tuple<> tuple = dbSorted[idAlias];
        // populate `db2` Sortedleaves
        Range<IdAlias> idAliasRange {idAlias, idAlias};
        Tuple<IdAlias> newTuple(tuple.getDbDoc(), idAliasRange);
        db2.push_back(newTuple);

        // populate `db1` leaves
        Kw kw = tuple.getKw();
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
    // make sure to write in last `Kw` (which cannot be detected by `kw != prevKw` in
    // the loop above; note this relies on nothing in `db` having keyword `DUMMY`)
    if (prevKw != DUMMY) {
        addDb1Leaf(prevKw, firstIdAliasWithKw, lastIdAliasWithKw);
    }

    // build TDAG 2 over id aliases
    // TODO move this process to a util (tdag util even? or no) function?
    // maybe even some padding stuff like nlogn or log src i* setup?
    IdAlias maxIdAlias = 0;
    for (Tuple<IdAlias> tuple : db2) {
        IdAlias idAlias = tuple.getDbKwRange().first; // must be size 1 range
        if (idAlias > maxIdAlias) {
            maxIdAlias = idAlias;
        }
    }
    this->tdag2 = new TdagNode<IdAlias>(Range<IdAlias> {0, maxIdAlias});

    // replicate every document to all id alias ranges/TDAG 2 nodes that cover it
    int64_t db2Size = db2.size();
    db2.reserve(utils::calcTdagTupleCount(db2Size));
    for (int64_t i = 0; i < db2Size; i++) {
        Tuple<IdAlias> tuple = db2[i];
        Range<IdAlias> idAliasRange = tuple.getDbKwRange();
        std::list<Range<IdAlias>> ancestors = this->tdag2->getLeafAncestors(idAliasRange);
        for (Range<IdAlias> ancestor : ancestors) {
            // ancestors include the leaf itself, which is already in `db2`
            if (ancestor == idAliasRange) {
                continue;
            }
            Tuple<IdAlias> newTuple(tuple.getDbDoc(), ancestor);
            db2.push_back(newTuple);
        }
    }

    this->underly2->setup(secParam, db2);

    //--------------------------------------------------------------------------
    // build index 1

    // build TDAG 1 over `Kw`s
    Range<Kw> db1KwBounds = utils::findDbKwBounds(db1);
    this->tdag1 = new TdagNode<Kw>(db1KwBounds);

    // replicate every document (in this case `SrcIDb1Tuple`s) to all keyword ranges/
    // TDAG 1 nodes that cover it
    int64_t db1Size = db1.size();
    db1.reserve(utils::calcTdagTupleCount(db1Size));
    for (int64_t i = 0; i < db1Size; i++) {
        SrcIDb1Tuple tuple = db1[i];
        Range<Kw> kwRange = tuple.getDbKwRange();
        std::list<Range<Kw>> ancestors = this->tdag1->getLeafAncestors(kwRange);
        for (Range<Kw> ancestor : ancestors) {
            if (ancestor == kwRange) {
                continue;
            }
            SrcIDb1Tuple newTuple(tuple.getDbDoc(), ancestor);
            db1.push_back(newTuple);
        }
    }

    this->underly1->setup(secParam, db1);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class LogSrcI<PiBas>;
template class LogSrcI<NLogN>;
