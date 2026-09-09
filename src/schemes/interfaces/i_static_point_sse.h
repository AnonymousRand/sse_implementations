#pragma once

#include <algorithm>
#include <concepts>
#include <vector>

#include "schemes/interfaces/i_sse.h"

#include "types/basic_types.h"
#include "types/enc_ind/enc_ind_types.h"
#include "types/range.h"
#include "types/tuple.h"
#include "types/ustring.h"

#include "utils/crypto.h"
#include "utils/misc.h"


// subclasses of this include `PiBas`, `NLogN`, and `log_src_i_star::Underly`
// provide shared code for `search()` (depending on `searchRaw()`)
template <IsDbTuple DbTuple = Tuple<>>
class IStaticPointSse : public virtual ISse<DbTuple> {
protected:
    using DbDoc = typename ISse<DbTuple>::DbDoc;
    using DbKw = typename ISse<DbTuple>::DbKw;

public:
    //--------------------------------------------------------------------------
    // shared code

    std::vector<DbDoc> search(
        const Range<DbKw>& query, bool shouldCleanUpResults = true, bool isNaive = true
    ) const override {
        std::vector<DbDoc> allResults;

        if (isNaive) {
            // naive, insecure range search: just individually query every point in range
            for (DbKw dbKw = query.first; dbKw <= query.second; dbKw++) {
                std::vector<DbDoc> results = this->searchRaw(Range {dbKw, dbKw});
                // (this uses move instead of copy)
                std::move(results.begin(), results.end(), std::back_inserter(allResults));
            }
        } else {
            // search entire range in one go (i.e. `query` itself must be in the db),
            // e.g. as the underlying scheme for a range scheme like Log-SRC
            allResults = this->searchRaw(query);
        }

        if (shouldCleanUpResults) {
            utils::misc::cleanUpResults(allResults);
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
    // helpers

    virtual std::vector<DbDoc> searchRaw(const Range<DbKw>& query) const = 0;
    
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
