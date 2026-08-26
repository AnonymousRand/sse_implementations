#include "schemes/sda/sda.h"

#include <algorithm>
#include <concepts>
#include <cmath>
#include <vector>

#include "schemes/interfaces/sd_underly.h"

// for explicit template instantiation
#include "schemes/log_src/log_src.h"
#include "schemes/log_src_i/log_src_i.h"
#include "schemes/log_src_i_star/log_src_i_star.h"
#include "schemes/n_log_n/n_log_n.h"
#include "schemes/pi_bas/pi_bas.h"

#include "utils/misc.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


template <IsSdUnderly Underly>
Sda<Underly>::~Sda() {
    this->clear();
}


//------------------------------------------------------------------------------
// `ISse`


template <IsSdUnderly Underly>
void Sda<Underly>::setup(int secParam, const Db<Tuple<>>& db) {
    this->clear();
    this->secParam = secParam;

    if (this->useShortcutSetup) {
        // need this special case to avoid things like indexing issues later
        if (db.getSize() == 0) {
            this->firstEmptyInd = 0;
            return;
        }

        bigint lastFilledInd = std::log2(db.getSize());

        // this is the shortcut way: simply initialize and fill in all subindexes in one go
        // (note that the non-shortcut `setup()` places earlier items in `db` into larger
        // subindexes, so we preserve that behavior here by starting from the earlier tuples
        // in `db` up the largest subindexes first (this was needed anyway))
        bigint dbPos = 0;
        for (bigint i = lastFilledInd; i >= 0; i--) {
            bigint indSize = (bigint)std::pow(2, i);
            Db<Tuple<>> indDb;
            if (dbPos < db.getSize()) {
                if (dbPos + indSize < db.getSize()) {
                    indDb = Db<Tuple<>>(db, dbPos, dbPos + indSize);
                } else {
                    indDb = Db<Tuple<>>(db, dbPos, db.getSize());
                }
            } else {
                indDb = Db<Tuple<>> {};
            }

            Underly* newUnderly = new Underly(this->benchmark);
            newUnderly->setup(this->secParam, indDb);
            this->underlys.push_back(newUnderly);
            dbPos += indSize;
        }
        // reverse the vector at the end as we had pushed smaller subindexes to the back
        std::reverse(this->underlys.begin(), this->underlys.end());

        // update the pointer to the first empty index as usual (like in `update()`)
        this->firstEmptyInd = this->calcFirstEmptyInd();
    } else {
        for (const Tuple<>& tuple : db) {
            this->update(tuple);
        }
    }
}


template <IsSdUnderly Underly>
std::vector<Tuple<>> Sda<Underly>::search(
    const Range<Kw>& query, bool shouldCleanUpResults, bool isNaive
) const {
    std::vector<Tuple<>> allResults;

    // search through all non-empty indexes
    for (Underly* underly : this->underlys) {
        if (underly->getSize() == 0) {
            continue;
        }
        // don't filter out deleted tuples in underlying schemes even if `shouldCleanUpResults`
        // is `true`; the cancellation tuple for a document is not guaranteed to be in
        // the same index as the inserting tuple, so we can't rely on the individual
        // underlying instances to filter out all deleted documents
        std::vector<Tuple<>> results = underly->search(query, false, isNaive);
        allResults.insert(allResults.end(), results.begin(), results.end());
    }

    if (shouldCleanUpResults) {
        allResults = utils::misc::cleanUpResults(allResults);
    }
    return allResults;
}


template <IsSdUnderly Underly>
void Sda<Underly>::clear() {
    // (apparently vector `clear()` automatically calls the destructor for each element
    // *unless* it is a pointer)
    for (Underly* underly : this->underlys) {
        if (underly != nullptr) {
            delete underly;
            underly = nullptr;
        }
    }
    this->underlys.clear();

    this->firstEmptyInd = 0;

    // clears `this->updateCount`
    IDsse<Tuple<>>::clear();
}


//------------------------------------------------------------------------------
// `IDsse`


template <IsSdUnderly Underly>
void Sda<Underly>::update(const Tuple<>& newTuple) {
    // if empty, initialize first index
    if (this->updateCount == 0) {
        Underly* newUnderly = new Underly(this->benchmark);
        newUnderly->setup(this->secParam, Db<Tuple<>> {newTuple});
        this->underlys.push_back(newUnderly);
        this->firstEmptyInd = 1;
        this->updateCount++;
        return;
    }

    // merge all EDB_<j into EDB_j where j is `this->firstEmptyInd`
    Db<Tuple<>> mergedDb;
    for (bigint i = 0; i < this->firstEmptyInd; i++) {
        // (`getDb()` appends to the passed-in container)
        this->underlys[i]->getDb(mergedDb);
    }
    mergedDb.append(newTuple);
    if (this->firstEmptyInd >= this->underlys.size() - 1) {
        // if we need to create a new, larger index
        Underly* newUnderly = new Underly(this->benchmark);
        newUnderly->setup(this->secParam, mergedDb);
        this->underlys.push_back(newUnderly);
    } else {
        this->underlys[this->firstEmptyInd]->setup(this->secParam, mergedDb);
    }

    // clear all EDB_<j
    for (bigint i = 0; i < this->firstEmptyInd; i++) {
        this->underlys[i]->clear();
    }

    // update the pointer to the first empty index
    this->firstEmptyInd = this->calcFirstEmptyInd();

    this->updateCount++;
}


//------------------------------------------------------------------------------
// helpers


template <IsSdUnderly Underly>
bigint Sda<Underly>::calcFirstEmptyInd() const {
    bigint newFirstEmpty = 0;
    // (note: the order of conditions here is important; we need the short-circuiting!)
    while (newFirstEmpty < this->underlys.size() && this->underlys[newFirstEmpty]->getSize() > 0) {
        newFirstEmpty++;
    }
    return newFirstEmpty;
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class Sda<PiBas<>>;
template class Sda<NLogN<>>;
template class Sda<LogSrc<PiBas>>;
template class Sda<LogSrc<NLogN>>;
template class Sda<LogSrcI<PiBas>>;
template class Sda<LogSrcI<NLogN>>;
template class Sda<LogSrcIStar>;
