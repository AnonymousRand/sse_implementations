#include "schemes/log_src_i_star/underly.h"

#include <cmath>
#include <concepts>
#include <cstdint>
#include <utility>
#include <vector>

#include "utils/doc.h"
#include "utils/enc_ind.h"
#include "utils/range.h"
#include "utils/sse_utils.h"
#include "utils/ustring.h"


namespace log_src_i_star {


//------------------------------------------------------------------------------
// `ISse`


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void Underly<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    Range<DbKw> dbKwBounds = utils::findDbKwBounds(db);
    this->leafCount = dbKwBounds.size();
    NLogN<DbDoc, DbKw>::setup(secParam, db);
}


//------------------------------------------------------------------------------
// `IStaticPointSse`


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
std::vector<DbDoc> Underly<DbDoc, DbKw>::searchBase(const Range<DbKw>& query) const {
    std::vector<DbDoc> results;

    // PRF(K_1, w)
    ustring queryToken = this->genQueryToken(query);

    // for Log-SRC-i*, the TDAG structure means we can determine the number of results
    // and hence the level to search based on the size of the queried range/node, so we
    // don't have to store an encrypted map (and result size is leaked to server anyway)
    int64_t dbKwCount = query.size();
    int64_t dbKwPaddedCount = std::pow(2, std::ceil(std::log2(dbKwCount))); // this is bucket size

    // compute `lvl` and `pos` of correct bucket (the same way as in `setup()`)
    ustring label;
    std::pair<uint64_t, uint64_t> lvlAndPos = this->map(queryToken, dbKwPaddedCount, label);
    uint64_t lvl = lvlAndPos.first;
    uint64_t pos = lvlAndPos.second;
    // return entire bucket (`dbKwPaddedCount` instead of `dbKwCount`) from server
    // to hide true result size
    uint64_t startPos = pos * this->computeBcktSizeOnLvl(lvl);
    std::vector<EncIndVal> encResults = this->server->searchEncIndForBckt(
        lvl, startPos, dbKwPaddedCount, label
    );
    for (EncIndVal encResult : encResults) {
        DbDoc result = this->decryptEncIndVal(encResult);
        results.push_back(result);
    }

    return results;
}


//------------------------------------------------------------------------------
// other


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
int64_t Underly<DbDoc, DbKw>::computeNumLvls() const {
    // the key to avoiding the blowup of using NLogN as a black box is by using
    // `leafCount` instead of `this->size` here, since `this->size` includes the
    // replicated tuples and using it sort of assumes those are only the "raw" entries
    return std::ceil(std::log2(this->leafCount)) + 1;
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
int64_t Underly<DbDoc, DbKw>::computeBcktCountOnLvl(int64_t lvlNum) const {
    if (lvlNum == 0) {
        return this->leafCount;
    } else {
        // this gives the TDAG-specific node/bucket count at level `lvlNum` (for `lvlNum` >= 1)
        return std::pow(2, this->numLvls - lvlNum) - 1;
    }
}


template class Underly<Doc<>, Kw>;       
template class Underly<SrcIDb1Doc, Kw>;
//template class Underly<Doc<IdAlias>, IdAlias>;


} // namespace `log_src_i_star`
