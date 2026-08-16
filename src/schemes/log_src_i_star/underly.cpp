#include "schemes/log_src_i_star/underly.h"

#include <cmath>
#include <concepts>
#include <utility>
#include <vector>

#include "utils/db/db.h"
#include "utils/enc_ind.h"
#include "utils/range.h"
#include "utils/tuple.h"
#include "utils/types.h"
#include "utils/ustring.h"


namespace log_src_i_star {


//------------------------------------------------------------------------------
// `ISse`


template <IsDbTuple DbTuple>
void Underly<DbTuple>::setup(int secParam, const Db<DbTuple>& db) {
    Range<DbKw> dbKwBounds = db.findDbKwBounds();
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
    ubigint startPos = pos * this->computeBcktSizeOnLvl(lvl);
    std::vector<EncIndVal> encResults = this->server->searchEncIndForBckt(
        lvl, startPos, dbKwPaddedCount, label
    );
    for (const EncIndVal& encResult : encResults) {
        DbTuple result = this->decryptEncIndVal(encResult);
        results.push_back(result);
    }

    return results;
}


//------------------------------------------------------------------------------
// helpers


template <IsDbTuple DbTuple>
bigint Underly<DbTuple>::computeNumLvls() const {
    // the key to avoiding the blowup of using NLogN as a black box is by using
    // `leafCount` instead of `this->size` here, since `this->size` includes the
    // replicated tuples and using it sort of assumes those are only the "raw" tuples
    return std::ceil(std::log2(this->leafCount)) + 1;
}


template <IsDbTuple DbTuple>
bigint Underly<DbTuple>::computeBcktCountOnLvl(bigint lvlNum) const {
    if (lvlNum == 0) {
        return this->leafCount;
    } else {
        // this gives the TDAG-specific node/bucket count at level `lvlNum` (for `lvlNum` >= 1)
        return std::pow(2, this->numLvls - lvlNum) - 1;
    }
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class Underly<Tuple<>>;       
template class Underly<SrcIDb1Tuple>;
//template class Underly<Tuple<IdAlias>>;


} // namespace `log_src_i_star`
