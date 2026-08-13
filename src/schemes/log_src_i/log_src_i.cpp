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
        // populate `db2` leaves
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
    // >>TODO move this process to a util (tdag util even? or no) function?
    // TODO maybe even move some padding stuff like nlogn or log src i* setup to utils?
    this->buildTdag2(db2);

    // replicate every document to all id alias ranges/TDAG 2 nodes that cover it
    this->replicateDb<>(db2, this->tdag2);

    this->underly2->setup(secParam, db2);

    //--------------------------------------------------------------------------
    // build index 1

    // build TDAG 1 over `Kw`s
    Range<Kw> db1KwBounds = utils::findDbKwBounds(db1);
    this->tdag1 = new TdagNode<Kw>(db1KwBounds);

    // replicate every document to all keyword ranges/TDAG 1 nodes that cover it
    this->replicateDb<>(db1, this->tdag1);

    this->underly1->setup(secParam, db1);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class LogSrcI<PiBas>;
template class LogSrcI<NLogN>;
