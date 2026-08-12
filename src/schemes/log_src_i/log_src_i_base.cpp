#include "schemes/log_src_i/log_src_i_base.h"

#include <concepts>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "schemes/interfaces/sse.h"

// for explicit template instantiation
#include "schemes/log_src_i_star/underly.h"
#include "schemes/n_log_n/n_log_n.h"
#include "schemes/pi_bas/pi_bas.h"

#include "utils/doc.h"
#include "utils/range.h"
#include "utils/sse_utils.h"
#include "utils/tdag.h"


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
    this->size = 0;
}


//------------------------------------------------------------------------------
// `ISdUnderly`


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
void LogSrcIBase<Underly>::getDb(Db<Doc<>, Kw>& ret) const {
    // reconstruct the original DB passed to `setup()` from Log-SRC-i's two indexes
    // (an alternative is to store the original DB in a separate PiBas instance and call
    // `getDb()` on that; it will be slightly faster but it will also take up more disk space,
    // and more importantly it can be another attack vector (e.g. it reveals the exact db size))
    Db<SrcIDb1Doc, Kw> db1;
    Db<Doc<IdAlias>, IdAlias> db2;
    this->underly1->getDb(db1);
    this->underly2->getDb(db2);
    Ind<IdAlias, Doc<IdAlias>> ind2 = utils::genInd(db2);

    for (DbEntry<SrcIDb1Doc, Kw> dbEntry : db1) {
        Range<Kw> kwRange = dbEntry.second;
        // only iterate through leaf nodes in DB 1
        if (kwRange.size() > 1) {
            continue;
        }

        // also exclude ALL types of dummies (this is done client-side so it's fine to reveal sizes)
        SrcIDb1Doc srcIDb1Doc = dbEntry.first;
        Range<IdAlias> idAliasRange = srcIDb1Doc.getIdAliasRange();
        if (idAliasRange == DUMMY_RANGE<IdAlias>()) {
            continue;
        }

        for (IdAlias idAlias = idAliasRange.first; idAlias < idAliasRange.second; idAlias++) {
            auto iter = ind2.find(Range<IdAlias> {idAlias, idAlias});
            if (iter == ind2.end()) {
                std::cerr << "Error: LogSrcIBase::getDb(): "
                          << "I don't think this is supposed to happen." << std::endl;
                std::exit(EXIT_FAILURE);
            }

            std::vector<Doc<IdAlias>> kwList = iter->second;
            for (Doc<IdAlias> doc : kwList) {
                ret.push_back(
                    DbEntry {Doc<> {doc.getId(), doc.getKw(), doc.getOp(), kwRange}, kwRange}
                );
            }
        }
    }
} 


template class LogSrcIBase<PiBas>;
template class LogSrcIBase<NLogN>;
template class LogSrcIBase<log_src_i_star::Underly>;
