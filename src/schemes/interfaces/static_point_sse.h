#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/sse.h"

#include "utils/benchmark.h"
#include "utils/crypto.h"
#include "utils/misc.h"
#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_utils.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


// subclasses of this include `PiBas`, `NLogN`, and `log_src_i_star::Underly`
// provide shared code for `search()` (depending on `searchBase()`)
template <IsDbTuple DbTuple = Tuple<>>
class IStaticPointSse : public virtual ISse<DbTuple> {
protected:
    using DbKw = typename ISse<DbTuple>::DbKw;

public:
    using ISse<DbTuple>::ISse;

    //--------------------------------------------------------------------------
    // shared code

    std::vector<DbTuple> search(
        const Range<DbKw>& query, bool shouldCleanUpResults = true, bool isNaive = true
    ) const override {
        std::vector<DbTuple> allResults;

        if (isNaive) {
            // naive, insecure range search: just individually query every point in range
            for (DbKw dbKw = query.first; dbKw <= query.second; dbKw++) {
                std::vector<DbTuple> results = this->searchBase(Range {dbKw, dbKw});
                allResults.insert(allResults.end(), results.begin(), results.end());
            }
        } else {
            // search entire range in one go (i.e. `query` itself must be in the db),
            // e.g. as the underlying scheme for a range scheme like Log-SRC
            allResults = this->searchBase(query);
        }

        if (shouldCleanUpResults) {
            allResults = utils::misc::cleanUpResults(allResults);
        }
        return allResults;
    }

    // handle clearing of this class' member variables
    void clear() override {
        this->prfKey = utils::ustr::toUstr("");
        this->encKey = utils::ustr::toUstr("");
    }

protected:
    ustring prfKey;
    ustring encKey;

    //--------------------------------------------------------------------------
    // methods to implement

    virtual std::vector<DbTuple> searchBase(const Range<DbKw>& query) const = 0;
    
    //--------------------------------------------------------------------------
    // shared code

    /**
     * helper function to decrypt `encIndVal`.
     */
    DbTuple decryptEncIndVal(const EncIndVal& encIndVal) const {
        ustring encDbTuple = encIndVal.first;
        ustring iv = encIndVal.second;
        ustring decDbTuple = utils::crypto::decryptAndUnpad(this->encKey, encDbTuple, iv);
        return DbTuple::fromUstr(decDbTuple);
    }
};
