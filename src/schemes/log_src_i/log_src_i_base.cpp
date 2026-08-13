#include "schemes/log_src_i/log_src_i_base.h"

#include <bit>
#include <cmath>
#include <concepts>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <utility>
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
#include "utils/types.h"


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>, Kw>>
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


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>, Kw>>
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


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>, Kw>>
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


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>, Kw>>
void LogSrcIBase<Underly>::getDb(Db<Tuple<>>& ret) const {
    // reconstruct the original DB passed to `setup()` from Log-SRC-i's two indexes
    // (an alternative is to store the original DB in a separate PiBas instance and call
    // `getDb()` on that; it will be slightly faster but it will also take up more disk space,
    // and more importantly it can be another attack vector (e.g. it reveals the exact db size))
    Db<SrcIDb1Tuple> db1;
    Db<Tuple<IdAlias>> db2;
    this->underly1->getDb(db1);
    this->underly2->getDb(db2);
    Ind<IdAlias, Tuple<IdAlias>> ind2 = utils::genInd(db2);

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

            std::vector<Tuple<IdAlias>> dbKwList = iter->second;
            for (Tuple<IdAlias> db2Tuple : dbKwList) {
                Tuple<> newTuple(db2Tuple.getDbDoc(), kwRange);
                ret.push_back(newTuple);
            }
        }
    }
} 


//------------------------------------------------------------------------------
// other


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>, Kw>>
Db<Tuple<>> LogSrcIBase<Underly>::sortInputDb(const Db<Tuple<>>& db) const {
    Db<Tuple<>> dbSorted = db;
    auto sortByKw = [](const Tuple<>& tuple1, const Tuple<>& tuple2) {
        return tuple1.getKw() < tuple2.getKw();
    };
    std::sort(dbSorted.begin(), dbSorted.end(), sortByKw);
    return dbSorted;
}


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>, Kw>>
std::pair<Db<SrcIDb1Tuple>, Db<Tuple<IdAlias>>> LogSrcIBase<Underly>::initDbLeaves(
    const Db<Tuple<>>& dbSorted,
    const std::function<
        void(Kw prevKw, IdAlias firstIdAliasWithKw, IdAlias lastIdAliasWithKw)
    >& addDb1Leaf
) {
    Db<SrcIDb1Tuple> db1;
    Db<Tuple<IdAlias>> db2;
    db1.reserve(dbSorted.size());
    db2.reserve(dbSorted.size());
    Kw prevKw = DUMMY;
    IdAlias firstIdAliasWithKw;
    IdAlias lastIdAliasWithKw;

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

    return std::pair {db1, db2};
}


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>, Kw>>
void LogSrcIBase<Underly>::buildTdag2(Db<Tuple<IdAlias>>& db2, bool shouldPadLeafCount) {
    IdAlias maxIdAlias = 0;
    for (Tuple<IdAlias> tuple: db2) {
        IdAlias idAlias = tuple.getDbKwRange().first; // must be size 1 range
        if (idAlias > maxIdAlias) {
            maxIdAlias = idAlias;
        }
    }

    if (shouldPadLeafCount) {
        this->padDb(db2, maxIdAlias);
    }

    this->tdag2 = new TdagNode<IdAlias>(0, maxIdAlias);
    this->replicateDb<>(db2, this->tdag2);
}


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>, Kw>>
template <IsDbTuple DbTuple>
void LogSrcIBase<Underly>::padDb(Db<DbTuple>& db, typename DbTuple::DbKwType currMaxDbKw) const {
    using DbKw = typename DbTuple::DbKwType;

    int64_t dbSize = db.size();
    if (!std::has_single_bit((uint64_t)dbSize)) {
        int64_t amountToPad = std::pow(2, std::ceil(std::log2(dbSize))) - dbSize;
        db.reserve(dbSize + amountToPad);
        for (int64_t i = 0; i < amountToPad; i++) {
            currMaxDbKw++;
            Range<DbKw> dbKwRange {currMaxDbKw, currMaxDbKw};
            DbTuple dummyTuple = DbTuple::genDummy(dbKwRange);
            db.push_back(dummyTuple);
        }
    }
}


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>, Kw>>
template <IsDbTuple DbTuple>
void LogSrcIBase<Underly>::replicateDb(
    Db<DbTuple>& db, const TdagNode<typename DbTuple::DbKwType>* tdag
) const {
    using DbKw = typename DbTuple::DbKwType;

    int64_t dbSize = db.size();
    db.reserve(utils::calcTdagTupleCount(dbSize));
    for (int64_t i = 0; i < dbSize; i++) {
        DbTuple tuple = db[i];
        Range<DbKw> dbKwRange = tuple.getDbKwRange();
        std::list<Range<DbKw>> ancestors = this->tdag1->getLeafAncestors(dbKwRange);
        for (Range<DbKw> ancestor : ancestors) {
            if (ancestor == dbKwRange) {
                continue;
            }
            DbTuple newTuple(tuple.getDbDoc(), ancestor);
            db.push_back(newTuple);
        }
    }
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class LogSrcIBase<PiBas>;
template class LogSrcIBase<NLogN>;
template class LogSrcIBase<log_src_i_star::Underly>;


// this is terribly ugly, but i don't think there's a better way ;-;
template void LogSrcIBase<PiBas>::padDb(Db<Tuple<>>& db, Kw currMaxDbKw);
template void LogSrcIBase<NLogN>::padDb(Db<Tuple<>>& db, Kw currMaxDbKw);
template void LogSrcIBase<log_src_i_star::Underly>::padDb(Db<Tuple<>>& db, Kw currMaxDbKw);

template void LogSrcIBase<PiBas>::padDb(Db<SrcIDb1Tuple>& db, Kw currMaxDbKw);
template void LogSrcIBase<NLogN>::padDb(Db<SrcIDb1Tuple>& db, Kw currMaxDbKw);
template void LogSrcIBase<log_src_i_star::Underly>::padDb(Db<SrcIDb1Tuple>& db, Kw currMaxDbKw);

//template void LogSrcIBase<PiBas>::padDb(Db<Tuple<IdAlias>>& db, IdAlias currMaxDbKw);
//template void LogSrcIBase<NLogN>::padDb(Db<Tuple<IdAlias>>& db, IdAlias currMaxDbKw);
//template void LogSrcIBase<log_src_i_star::Underly>::padDb(
//    Db<Tuple<IdAlias>>& db, IdAlias currMaxDbKw
//);


template void LogSrcIBase<PiBas>::replicateDb(Db<Tuple<>>& db, const TdagNode<Kw>* tdag);
template void LogSrcIBase<NLogN>::replicateDb(Db<Tuple<>>& db, const TdagNode<Kw>* tdag);
template void LogSrcIBase<log_src_i_star::Underly>::replicateDb(
    Db<Tuple<>>& db, const TdagNode<Kw>* tdag
);

template void LogSrcIBase<PiBas>::replicateDb(Db<SrcIDb1Tuple>& db, const TdagNode<Kw>* tdag);
template void LogSrcIBase<NLogN>::replicateDb(Db<SrcIDb1Tuple>& db, const TdagNode<Kw>* tdag);
template void LogSrcIBase<log_src_i_star::Underly>::replicateDb(
    Db<SrcIDb1Tuple>& db, const TdagNode<Kw>* tdag
);

//template void LogSrcIBase<PiBas>::replicateDb(
//    Db<Tuple<IdAlias>>& db, const TdagNode<IdAlias>* tdag
//);
//template void LogSrcIBase<NLogN>::replicateDb(
//    Db<Tuple<IdAlias>>& db, const TdagNode<IdAlias>* tdag
//);
//template void LogSrcIBase<log_src_i_star::Underly>::replicateDb(
//    Db<Tuple<IdAlias>>& db, const TdagNode<IdAlias>* tdag
//);
