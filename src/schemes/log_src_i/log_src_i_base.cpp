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

#include "utils/db.h"
#include "utils/range.h"
#include "utils/sse_utils.h"
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
    if (src1 == DUMMY_RANGE<Kw>()) { 
        return std::vector<Tuple<>> {};
    }
    std::vector<SrcIDb1Tuple> query1Results = this->underly1->search(src1, false, false);

    //--------------------------------------------------------------------------
    // query 2

    // generate query for query 2 based on query 1 results
    // (filter out unnecessary choices and merge remaining ones into a single id range)
    IdAlias minIdAlias = DUMMY;
    IdAlias maxIdAlias = DUMMY;
    for (SrcIDb1Tuple query1Result : query1Results) {
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
    if (src2 == DUMMY_RANGE<IdAlias>()) {
        return std::vector<Tuple<>> {};
    }
    return this->underly2->search(src2, shouldCleanUpResults, false);
}


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
void LogSrcIBase<Underly>::clear() {
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
    this->size = 0;
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
    Ind<Tuple<IdAlias>> ind2 = utils::genInd(db2);

    for (SrcIDb1Tuple db1Tuple : db1) {
        Range<Kw> kwRange = db1Tuple.getDbKwRange();
        // only iterate through leaf nodes in DB 1
        if (kwRange.size() > 1) {
            continue;
        }
        // also exclude ALL types of dummies (this is done client-side so it's fine to reveal sizes)
        Range<IdAlias> idAliasRange = db1Tuple.getIdAliasRange();
        if (idAliasRange == DUMMY_RANGE<IdAlias>()) {
            continue;
        }

        for (IdAlias idAlias = idAliasRange.first; idAlias <= idAliasRange.second; idAlias++) {
            auto iter = ind2.find(Range<IdAlias> {idAlias, idAlias});
            if (iter == ind2.end()) {
                std::cerr << "Error: LogSrcIBase::getDb(): "
                          << "I don't think this is supposed to happen." << std::endl;
                std::exit(EXIT_FAILURE);
            }

            Db<Tuple<IdAlias>> dbKwList = iter->second;
            for (Tuple<IdAlias> db2Tuple : dbKwList) {
                Tuple<> newTuple(db2Tuple.getDbDoc(), kwRange);
                ret.push_back(newTuple);
            }
        }
    }
} 


//------------------------------------------------------------------------------
// other


// TODO put this into sse utils?
template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
Db<Tuple<>> LogSrcIBase<Underly>::sortInputDb(const Db<Tuple<>>& db) {
    // sort this vector of indices in memory instead of the actual tuples on disk, which is
    // probably a lot faster and also avoids having to make a random-access iterator for `Db`
    // (which is basically impossible since that requires dereferencing the iterator to return
    // a reference to a tuple object, but tuple objects are only constructed on the fly and are
    // not actually stored anywhere; they are encoded into a string when stored on the disk)
    std::vector<int64_t> dbIndices;
    dbIndices.reserve(db.size());
    for (int64_t index = 0; index < db.size(); index++) {
        dbIndices.push_back(index);
    }

    auto sortIndicesByKw = [&db](int64_t index1, int64_t index2) {
        return db[index1].getKw() < db[index2].getKw();
    };
    std::sort(dbIndices.begin(), dbIndices.end(), sortIndicesByKw);

    // now build new sorted DB using this sorted vector of indices
    Db<Tuple<>> sortedDb;
    for (int64_t index : dbIndices) {
        sortedDb.push_back(db[index]);
    }
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

    for (int64_t idAlias = 0; idAlias < sortedDb.size(); idAlias++) {
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
