#include "schemes/log_src_i/log_src_i_base.h"

#include <concepts>
#include <vector>

#include "schemes/interfaces/sse.h"
#include "schemes/pi_bas/pi_bas.h"

#include "utils/doc.h"
#include "utils/range.h"
#include "utils/sse_utils.h"
#include "utils/tdag.h"

// for explicit template instantiation
#include "schemes/log_src_i_star/underly.h"
#include "schemes/n_log_n/n_log_n.h"
#include "schemes/pi_bas/pi_bas.h"


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
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
    if (this->origDbUnderly != nullptr) {
        delete this->origDbUnderly;
        this->origDbUnderly = nullptr;
    }
}


//------------------------------------------------------------------------------
// `ISse`


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
std::vector<Doc<>> LogSrcIBase<Underly>::search(
    const Range<Kw>& query, bool shouldCleanUpResults, bool isNaive
) const {
    //--------------------------------------------------------------------------
    // query 1

    Range<Kw> src1 = this->tdag1->findSrc(query);
    if (src1 == DUMMY_RANGE<Kw>()) { 
        return std::vector<Doc<>> {};
    }
    std::vector<SrcIDb1Doc> choices = this->underly1->search(src1, false, false);

    //--------------------------------------------------------------------------
    // query 2

    // generate query for query 2 based on query 1 results
    // (filter out unnecessary choices and merge remaining ones into a single id range)
    IdAlias minIdAlias = DUMMY;
    IdAlias maxIdAlias = DUMMY;
    for (SrcIDb1Doc choice : choices) {
        Kw choiceKw = choice.get().first;
        if (!query.contains(choiceKw)) {
            continue;
        }
        Range<IdAlias> choiceIdAliasRange = choice.get().second;
        if (choiceIdAliasRange.first < minIdAlias || minIdAlias == DUMMY) {
            minIdAlias = choiceIdAliasRange.first;
        }
        if (choiceIdAliasRange.second > maxIdAlias || maxIdAlias == DUMMY) {
            maxIdAlias = choiceIdAliasRange.second;
        }
    }
    // if there are no choices or something went wrong
    if (minIdAlias == DUMMY || maxIdAlias == DUMMY) {
        return std::vector<Doc<>> {};
    }

    // perform query 2
    Range<IdAlias> query2 {minIdAlias, maxIdAlias};
    Range<IdAlias> src2 = this->tdag2->findSrc(query2);
    if (src2 == DUMMY_RANGE<IdAlias>()) {
        return std::vector<Doc<>> {};
    }
    return this->underly2->search(src2, shouldCleanUpResults, false);
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
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
    this->origDbUnderly->clear();
    this->size = 0;
}


//------------------------------------------------------------------------------
// `ISdUnderly`


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
void LogSrcIBase<Underly>::getDb(Db<Doc<>, Kw>& ret) const {
    this->origDbUnderly->getDb(ret);
}


template class LogSrcIBase<PiBas>;
template class LogSrcIBase<NLogN>;
template class LogSrcIBase<log_src_i_star::Underly>;
