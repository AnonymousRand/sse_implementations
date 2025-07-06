#include "sda.h"

#include "log_src.h"
#include "log_src_i.h"
#include "log_src_i_loc.h"
#include "pi_bas.h"


template <IsSdaUnderlySse Underly>
Sda<Underly>::~Sda() {
    this->clear();
}


template <IsSdaUnderlySse Underly>
void Sda<Underly>::setup(int secParam, const Db<Doc, Kw>& db) {
    this->clear();

    this->secParam = secParam;
    for (DbEntry<Doc, Kw> entry : db) {
        this->update(entry);
    }
}


template <IsSdaUnderlySse Underly>
void Sda<Underly>::update(const DbEntry<Doc, Kw>& newEntry) {
    // if empty, initialize first index
    if (this->underlys.empty()) {
        Underly* newUnderly = new Underly();
        newUnderly->setup(this->secParam, Db<Doc, Kw> {newEntry});
        this->underlys.push_back(newUnderly);
        this->firstEmptyInd = 1;
        return;
    }

    // merge all EDB_<j into EDB_j where j is `this->firstEmptyInd`; always merge/insert into first index if it's empty
    Db<Doc, Kw> mergedDb;
    mergedDb.reserve(std::pow(2, this->firstEmptyInd));
    for (long i = 0; i < (this->firstEmptyInd < 1 ? 1 : this->firstEmptyInd); i++) {
        // original paper fetches encrypted index and decrypts instead of `getDb()`
        // but that could get messy with Log-SRC-i as it has two indexes
        // instead we just store and get plaintext DB for convenience of implementation
        Db<Doc, Kw> underlyDb = this->underlys[i]->getDb();
        mergedDb.insert(mergedDb.end(), underlyDb.begin(), underlyDb.end());
    }
    mergedDb.push_back(newEntry);
    if (this->firstEmptyInd < this->underlys.size() - 1) {
        this->underlys[this->firstEmptyInd]->setup(this->secParam, mergedDb);
    } else {
        Underly* newUnderly = new Underly();
        newUnderly->setup(this->secParam, mergedDb);
        this->underlys.push_back(newUnderly);
    }

    // clear all EDB_<j
    for (long i = 0; i < this->firstEmptyInd; i++) {
        this->underlys[i]->clear();
    }

    // update the pointer to the first empty index
    long firstEmpty;
    for (firstEmpty = 0; firstEmpty < this->underlys.size(); firstEmpty++) {
        if (this->underlys[firstEmpty]->isEmpty()) {
            break;
        }
    }
    this->firstEmptyInd = firstEmpty;
}


template <IsSdaUnderlySse Underly>
std::vector<Doc> Sda<Underly>::search(const Range<Kw>& query, bool shouldCleanUpResults, bool isNaive) const {
    std::vector<Doc> allResults;

    // search through all non-empty indexes
    for (Underly* underly : this->underlys) {
        if (underly->isEmpty()) {
            continue;
        }
        // don't filter out deleted tuples in underlying schemes even if `shouldCleanUpResults` is `true`
        // the cancellation tuple for a document is not guaranteed to be in same index as the inserting tuple
        // so we can't rely on the individual underlying instances to filter out all deleted documents
        std::vector<Doc> results = underly->search(query, false, isNaive);
        allResults.insert(allResults.end(), results.begin(), results.end());
    }
    if (shouldCleanUpResults) {
        cleanUpResults(allResults);
    }

    return allResults;
}


template <IsSdaUnderlySse Underly>
void Sda<Underly>::clear() {
    // apparently vector `clear()` automatically calls the destructor for each element *unless* it is a pointer
    for (Underly* underly : this->underlys) {
        if (underly != nullptr) {
            delete underly;
            underly = nullptr;
        }
    }
    this->underlys.clear();
}


template class Sda<PiBas<>>;
template class Sda<LogSrc<PiBas>>;
template class Sda<LogSrcI<PiBas>>;
template class Sda<LogSrcILoc<PiBasLoc>>;
