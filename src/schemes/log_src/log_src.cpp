#include "schemes/log_src/log_src.h"

#include <concepts>
#include <list>
#include <utility>
#include <vector>

#include "schemes/interfaces/sse.h"

// for explicit template instantiation
#include "schemes/n_log_n/n_log_n.h"
#include "schemes/pi_bas/pi_bas.h"

#include "utils/db.h"
#include "utils/range.h"
#include "utils/sse_utils.h"
#include "utils/tdag.h"
#include "utils/types.h"


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>, Kw>>
LogSrc<Underly>::~LogSrc() {
    this->clear();
    if (this->underly != nullptr) {
        delete this->underly;
        this->underly = nullptr;
    }
}


//------------------------------------------------------------------------------
// `ISse`


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>, Kw>>
void LogSrc<Underly>::setup(int secParam, Db<Tuple<>>& db) {
    this->clear();

    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();

    //--------------------------------------------------------------------------
    // build index

    // build TDAG over `Kw`s and replicate `db` appropriately
    utils::buildTdag(this->tdag, db);

    this->underly->setup(secParam, db);
}


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>, Kw>>
std::vector<Tuple<>> LogSrc<Underly>::search(
    const Range<Kw>& query, bool shouldCleanUpResults, bool isNaive
) const {
    Range<Kw> src = this->tdag->findSrc(query);
    if (src == DUMMY_RANGE<Kw>()) {
        return std::vector<Tuple<>> {};
    }
    return this->underly->search(src, shouldCleanUpResults, false);
}


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>, Kw>>
void LogSrc<Underly>::clear() {
    this->underly->clear();
    // delete TDAG fully since it is reallocated with `new` in `setup()`
    if (this->tdag != nullptr) {
        delete this->tdag;
        this->tdag = nullptr;
    }
    this->size = 0;
}


//------------------------------------------------------------------------------
// `ISdUnderly`


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>, Kw>>
void LogSrc<Underly>::getDb(Db<Tuple<>>& ret) const {
    // need to exclude replicated tuples: assume any tuples with `DbKw` range size >1 is replicated
    // (this doesn't seem to incur noticeable performance overhead with compiler optimizations)
    Db<Tuple<>> retWithRepls;
    this->underly->getDb(retWithRepls);
    for (Tuple<> tuple : retWithRepls) {
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
