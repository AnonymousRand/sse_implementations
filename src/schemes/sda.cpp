#include "schemes/sda.h"

#include <algorithm>
#include <cmath>

// for explicit template instantiation
#include "schemes/log_src.h"
#include "schemes/log_src_i.h"
#include "schemes/log_src_i_star.h"
#include "schemes/n_log_n.h"
#include "schemes/pi_bas.h"


template <IsSdaUnderly Underly>
Sda<Underly>::~Sda() {
    this->clear();
}


//------------------------------------------------------------------------------
// `ISse`


template <IsSdaUnderly Underly>
void Sda<Underly>::setup(int secParam, const Db<Doc<>, Kw>& db) {
    this->clear();
    this->secParam = secParam;

    if (this->useShortcutSetup) {
        int64_t lastFilledInd = db.size() != 0 ? (int64_t)std::log2(db.size()) : -1;

        // this is the shortcut way: simply initialize and fill in all subindexes in one go
        // (note that the non-shortcut `setup()` places earlier items in `db` into larger
        // subindexes, so we preserve that behavior here by starting from the earlier entries
        // in `db` up the largest subindexes first (this was needed anyway))
        int64_t dbPos = 0;
        for (int64_t i = lastFilledInd; i >= 0; i--) {
            int64_t indSize = (int64_t)std::pow(2, i);
            Db<Doc<>, Kw> indDb;
            if (dbPos < db.size()) {
                indDb = Db<Doc<>, Kw>(db.begin() + dbPos, db.begin() + dbPos + indSize);
            } else {
                indDb = Db<Doc<>, Kw>();
            }

            Underly* newUnderly = new Underly(this->benchmark);
            newUnderly->setup(this->secParam, indDb);
            this->underlys.push_back(newUnderly);
            dbPos += indSize;
        }
        // reverse the vector at the end as we had pushed smaller subindexes to the back
        std::reverse(this->underlys.begin(), this->underlys.end());

        // update the pointer to the first empty index as usual (like in `update()`)
        int64_t newFirstEmpty = 0;
        while (newFirstEmpty < this->underlys.size() && this->underlys[newFirstEmpty]->getSize() > 0) {
            newFirstEmpty++;
        }
        this->firstEmptyInd = newFirstEmpty;
    } else {
        for (DbEntry<Doc<>, Kw> entry : db) {
            this->update(entry);
        }
    }
}


template <IsSdaUnderly Underly>
std::vector<Doc<>> Sda<Underly>::search(const Range<Kw>& query, bool shouldCleanUpResults, bool isNaive) const {
    std::vector<Doc<>> allResults;

    // search through all non-empty indexes
    int64_t tmp = 0;
    for (Underly* underly : this->underlys) {
        std::cerr << "++++++ SDa searching underly " << tmp << "; it has size " << underly->getSize() << std::endl;
        tmp++;
        if (underly->getSize() == 0) {
            continue;
        }
        // don't filter out deleted tuples in underlying schemes even if `shouldCleanUpResults` is `true`
        // the cancellation tuple for a document is not guaranteed to be in same index as the inserting tuple
        // so we can't rely on the individual underlying instances to filter out all deleted documents
        std::vector<Doc<>> results = underly->search(query, false, isNaive);
        allResults.insert(allResults.end(), results.begin(), results.end());
    }

    if (shouldCleanUpResults) {
        cleanUpResults(allResults);
    }
    return allResults;
}


template <IsSdaUnderly Underly>
void Sda<Underly>::clear() {
    // apparently vector `clear()` automatically calls the destructor for each element *unless* it is a pointer
    for (Underly* underly : this->underlys) {
        if (underly != nullptr) {
            delete underly;
            underly = nullptr;
        }
    }
    this->underlys.clear();
    this->firstEmptyInd = 0;
}


//------------------------------------------------------------------------------
// `IDsse`


template <IsSdaUnderly Underly>
void Sda<Underly>::update(const DbEntry<Doc<>, Kw>& newDbEntry) {
    // if empty, initialize first index
    if (this->underlys.empty()) {
        Underly* newUnderly = new Underly(this->benchmark);
        newUnderly->setup(this->secParam, Db<Doc<>, Kw> {newDbEntry});
        this->underlys.push_back(newUnderly);
        this->firstEmptyInd = 1;
        return;
    }

    // merge all EDB_<j into EDB_j where j is `this->firstEmptyInd`; always merge/insert into first index if it's empty
    Db<Doc<>, Kw> mergedDb;
    mergedDb.reserve(std::pow(2, this->firstEmptyInd));
    std::cerr << "====== updating: " << newDbEntry.first << std::endl;
    for (int64_t i = 0; i < (this->firstEmptyInd < 1 ? 1 : this->firstEmptyInd); i++) {
        std::cerr << "trying to get subindex " << i << "; firstEmptyInd is " << this->firstEmptyInd << std::endl;
        // (`getDb()` appends to the passed-in container)
        this->underlys[i]->getDb(mergedDb);
        std::cerr << "merged subindex " << i << ", mergedDb is now, " << newDbEntry.first;
        for (auto entry : mergedDb) {
            std::cerr << ", " << entry.first;
        }
        std::cerr << std::endl;
    }
    mergedDb.push_back(newDbEntry);
    std::cerr << "making subindex " << this->firstEmptyInd << std::endl;
    if (this->firstEmptyInd >= this->underlys.size() - 1) {
        // if we need to create a new, larger index
        Underly* newUnderly = new Underly(this->benchmark);
        newUnderly->setup(this->secParam, mergedDb);
        this->underlys.push_back(newUnderly);
    } else {
        this->underlys[this->firstEmptyInd]->setup(this->secParam, mergedDb);
    }

    // clear all EDB_<j
    for (int64_t i = 0; i < this->firstEmptyInd; i++) {
        this->underlys[i]->clear();
    }

    // update the pointer to the first empty index
    int64_t newFirstEmpty = 0;
    while (newFirstEmpty < this->underlys.size() && this->underlys[newFirstEmpty]->getSize() > 0) {
        newFirstEmpty++;
    }
    this->firstEmptyInd = newFirstEmpty;
    std::cerr << std::endl;
}


template class Sda<PiBas<>>;
template class Sda<NLogN<>>;
template class Sda<LogSrc<PiBas>>;
template class Sda<LogSrc<NLogN>>;
template class Sda<LogSrcI<PiBas>>;
template class Sda<LogSrcI<NLogN>>;
template class Sda<LogSrcIStar>;
