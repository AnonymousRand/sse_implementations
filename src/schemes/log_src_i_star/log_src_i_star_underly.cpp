#include "schemes/log_src_i_star/log_src_i_star_underly.h"

#include <cmath>
#include <concepts>
#include <utility>
#include <vector>

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/enc_ind.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


namespace log_src_i_star {


//------------------------------------------------------------------------------
// `ISse`


template <IsDbTuple DbTuple>
void Underly<DbTuple>::setup(int secParam, const Db<DbTuple>& db) {
    Range<DbKw> dbKwBounds = db.getDbKwBounds();
    std::cout << "Log-SRC-i* setup() with kw bounds " << dbKwBounds << std::endl;
    // remember to not use `db.size()` as TDAG leaves must be contiguous!
    this->leafCount = dbKwBounds.size();
    NLogN<DbTuple>::setup(secParam, db);
}


//------------------------------------------------------------------------------
// `IStaticPointSse`


template <IsDbTuple DbTuple>
std::vector<DbTuple> Underly<DbTuple>::searchBase(const Range<DbKw>& query) const {
    std::vector<DbTuple> results;

    // PRF(K_1, w)
    ustring queryToken = this->genQueryToken(query);

    // for Log-SRC-i*, the number of results from either index and hence the level
    // to search is exactly the size of the queried range/SRC node, so we don't have to
    // additionally store an encrypted map (and result size is leaked to server anyway)
    bigint dbKwCount = query.size();
    bigint dbKwPaddedCount = std::pow(2, std::ceil(std::log2(dbKwCount))); // this is bucket size

    // compute `lvl` and `pos` of correct bucket (the same way as in `setup()`)
    ustring label;
    std::pair<ubigint, ubigint> lvlAndPos = this->map(queryToken, dbKwPaddedCount, label);
    ubigint lvl = lvlAndPos.first;
    ubigint pos = lvlAndPos.second;
    // return entire bucket (`dbKwPaddedCount` instead of `dbKwCount`) from server
    // to hide true result size
    ubigint startPos = pos * this->calcBcktSizeOnLvl(lvl);
    bigint bcktCountOnLvl = this->calcBcktCountOnLvl(lvl);
    std::vector<EncIndVal> encResults = this->server->searchEncIndForBckt(
        lvl, startPos, dbKwPaddedCount, bcktCountOnLvl, label
    );

    // decrypt results on the client
    results.reserve(encResults.size());
    for (const EncIndVal& encResult : encResults) {
        DbTuple result = this->decryptEncIndVal(encResult);
        results.push_back(result);
    }

    return results;
}


//------------------------------------------------------------------------------
// helpers


template <IsDbTuple DbTuple>
bigint Underly<DbTuple>::calcLvlCount() const {
    // the key to avoiding the blowup of using NLogN as a black box is by using
    // `leafCount` instead of `this->size` here, since `this->size` includes the
    // replicated tuples and using it sort of assumes those are only the "raw" tuples
    return std::ceil(std::log2(this->leafCount)) + 1;
}


template <IsDbTuple DbTuple>
bigint Underly<DbTuple>::calcBcktCountOnLvl(bigint lvl) const {
    if (lvl == 0) {
        return this->leafCount;
    } else {
        // this gives the TDAG-specific node/bucket count at level `lvl` (for `lvl` >= 1)
        return std::pow(2, this->lvlCount - lvl) - 1;
    }
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class Underly<Tuple<>>;       
template class Underly<SrcIDb1Tuple>;
//template class Underly<Tuple<IdAlias>>;


} // namespace `log_src_i_star`
