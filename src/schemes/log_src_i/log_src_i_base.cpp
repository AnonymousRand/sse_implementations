#include "schemes/log_src_i/log_src_i_base.h"

#include <algorithm>
#include <concepts>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <vector>

#include "schemes/interfaces/sse.h"

// for explicit template instantiation
#include "schemes/log_src_i_star/underly.h"
#include "schemes/n_log_n/n_log_n.h"
#include "schemes/pi_bas/pi_bas.h"

#include "utils/db/db.h"
#include "utils/ind.h"
#include "utils/range.h"
#include "utils/tdag.h"
#include "utils/tuple.h"
#include "utils/types.h"


//==============================================================================
// `LogSrcIBase`
//==============================================================================


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
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


//------------------------------------------------------------------------------
// `ISse`


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
std::vector<Tuple<>> LogSrcIBase<Underly>::search(
    const Range<Kw>& query, bool shouldCleanUpResults, bool isNaive
) const {
    //--------------------------------------------------------------------------
    // query 1

    Range<Kw> src1 = this->tdag1->findSrc(query);
    if (src1 == Range<Kw>::DUMMY()) { 
        return std::vector<Tuple<>> {};
    }
    std::vector<SrcIDb1Tuple> query1Results = this->underly1->search(src1, false, false);

    //--------------------------------------------------------------------------
    // query 2

    // generate query for query 2 based on query 1 results
    // (filter out unnecessary choices and merge remaining ones into a single id range)
    IdAlias minIdAlias = DUMMY;
    IdAlias maxIdAlias = DUMMY;
    for (const SrcIDb1Tuple& query1Result : query1Results) {
        Kw kw = query1Result.getKw();
        if (!query.contains(kw)) {
            continue;
        }
        Range<IdAlias> idAliasRange = query1Result.getIdAliasRange();
        if (idAliasRange.first < minIdAlias || minIdAlias == DUMMY) {
            minIdAlias = idAliasRange.first;
        }
        if (idAliasRange.second > maxIdAlias || maxIdAlias == DUMMY) {
            maxIdAlias = idAliasRange.second;
        }
    }
    // if there are no choices or something went wrong
    if (minIdAlias == DUMMY || maxIdAlias == DUMMY) {
        return std::vector<Tuple<>> {};
    }

    // perform query 2
    Range<IdAlias> query2 {minIdAlias, maxIdAlias};
    Range<IdAlias> src2 = this->tdag2->findSrc(query2);
    if (src2 == Range<IdAlias>::DUMMY()) {
        return std::vector<Tuple<>> {};
    }
    return this->underly2->search(src2, shouldCleanUpResults, false);
}


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
void LogSrcIBase<Underly>::clear() {
    // clears `this->size`
    ISdUnderly<Tuple<>>::clear();

    this->underly1->clear();
    this->underly2->clear();

    if (this->tdag1 != nullptr) {
        delete this->tdag1;
        this->tdag1 = nullptr;
    }
    if (this->tdag2 != nullptr) {
        delete this->tdag2;
        this->tdag2 = nullptr;
    }
}


//------------------------------------------------------------------------------
// `ISdUnderly`


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
void LogSrcIBase<Underly>::getDb(Db<Tuple<>>& ret) const {
    // reconstruct the original DB passed to `setup()` from Log-SRC-i's two indexes
    // (an alternative is to store the original DB in a separate PiBas instance and call
    // `getDb()` on that; it will be slightly faster but it will also take up more disk space,
    // and more importantly it can be another attack vector (e.g. it reveals the exact db size))
    Db<SrcIDb1Tuple> db1;
    Db<Tuple<IdAlias>> db2;
    this->underly1->getDb(db1);
    this->underly2->getDb(db2);
    Ind<Tuple<IdAlias>> ind2(db2);

    for (const SrcIDb1Tuple& db1Tuple : db1) {
        Range<Kw> kwRange = db1Tuple.getDbKwRange();
        // only iterate through leaf nodes in DB 1
        if (kwRange.size() > 1) {
            continue;
        }
        // also exclude ALL dummies (this is done client-side so it's fine to reveal sizes)
        Range<IdAlias> idAliasRange = db1Tuple.getIdAliasRange();
        if (idAliasRange == Range<IdAlias>::DUMMY()) {
            continue;
        }

        for (IdAlias idAlias = idAliasRange.first; idAlias <= idAliasRange.second; idAlias++) {
            Range<IdAlias> idAliasRange {idAlias, idAlias};
            auto iter = ind2.find(idAliasRange);
            if (iter == ind2.end()) {
                std::cerr << "Error: LogSrcIBase::getDb(): id alias range " << idAliasRange
                          << " not found in index 2" << std::endl;
                std::exit(EXIT_FAILURE);
            }

            Db<Tuple<IdAlias>> dbKwList = std::move(iter->second);
            for (const Tuple<IdAlias>& db2Tuple : dbKwList) {
                Tuple<> newTuple(db2Tuple.getDbDoc(), kwRange);
                ret.push_back(newTuple);
            }
        }
    }
} 


//------------------------------------------------------------------------------
// helpers


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
Db<Tuple<>> LogSrcIBase<Underly>::sortInputDb(const Db<Tuple<>>& db) {
    Db<Tuple<>> sortedDb = db;

    auto sortByKw = [](const Tuple<>& tuple1, const Tuple<>& tuple2) {
        return tuple1.getKw() < tuple2.getKw();
    };
    sortedDb.sort(sortByKw);
    return sortedDb;
}


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
void LogSrcIBase<Underly>::initDbsLeaves(
    const Db<Tuple<>>& sortedDb,
    Db<Tuple<IdAlias>>& db2,
    const std::function<
        void(Kw prevKw, IdAlias firstIdAliasWithKw, IdAlias lastIdAliasWithKw)
    >& addDb1Leaf
) {
    Kw prevKw = DUMMY;
    IdAlias firstIdAliasWithKw;
    IdAlias lastIdAliasWithKw;

    for (bigint idAlias = 0; idAlias < sortedDb.size(); idAlias++) {
        Tuple<> tuple = sortedDb[idAlias];
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
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class LogSrcIBase<PiBas>;
template class LogSrcIBase<NLogN>;
template class LogSrcIBase<log_src_i_star::Underly>;
