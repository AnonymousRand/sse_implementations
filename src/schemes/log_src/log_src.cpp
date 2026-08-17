#include "schemes/log_src/log_src.h"

#include <concepts>
#include <vector>

#include "schemes/interfaces/sse.h"
#include "schemes/log_src/log_src_utils.h"

// for explicit template instantiation
#include "schemes/n_log_n/n_log_n.h"
#include "schemes/pi_bas/pi_bas.h"

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tdag.h"
#include "utils/types/tuple.h"


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
LogSrc<Underly>::~LogSrc() {
    this->clear();
    if (this->underly != nullptr) {
        delete this->underly;
        this->underly = nullptr;
    }
}


//------------------------------------------------------------------------------
// `ISse`


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
void LogSrc<Underly>::setup(int secParam, const Db<Tuple<>>& db) {
    this->clear();

    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();

    //--------------------------------------------------------------------------
    // build index

    // build TDAG over `Kw`s and replicate `db` appropriately
    Db<Tuple<>> dbWithRepls = db;
    log_src::utils::buildTdagDbFromLeaves<Tuple<>>(dbWithRepls, this->tdag);

    this->underly->setup(secParam, dbWithRepls);
}


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
std::vector<Tuple<>> LogSrc<Underly>::search(
    const Range<Kw>& query, bool shouldCleanUpResults, bool isNaive
) const {
    Range<Kw> src = this->tdag->findSrc(query);
    if (src == Range<Kw>::DUMMY()) {
        return std::vector<Tuple<>> {};
    }
    return this->underly->search(src, shouldCleanUpResults, false);
}


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
void LogSrc<Underly>::clear() {
    // clears `this->size`
    ISdUnderly<Tuple<>>::clear();

    this->underly->clear();

    // delete TDAG fully since it is reallocated with `new` in `setup()`
    if (this->tdag != nullptr) {
        delete this->tdag;
        this->tdag = nullptr;
    }
}


//------------------------------------------------------------------------------
// `ISdUnderly`


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
void LogSrc<Underly>::getDb(Db<Tuple<>>& ret) const {
    // we only return leaves as that is what was originally passed to `setup()`, so we exclude
    // replicated tuples: assume any tuples with `DbKw` range size >1 is replicated and not a leaf
    Db<Tuple<>> underlyDb;
    underlyDb.reserve(utils::tdag::calcTdagTupleCount(this->size));
    this->underly->getDb(underlyDb);
    for (const Tuple<>& tuple : underlyDb) {
        Range<Kw> kwRange = tuple.getDbKwRange();
        if (kwRange.size() == 1) {
            ret.push_back(tuple);
        };
    }
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class LogSrc<PiBas>;
template class LogSrc<NLogN>;
