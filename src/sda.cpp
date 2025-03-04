#include "sda.h"

template <IMainDbDoc_ DbDoc, class DbKw, template<class ...> class Underly> requires IUnderly_<Underly, DbDoc, DbKw>
void SdaBase<DbDoc, DbKw, Underly>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->secParam = secParam;
    for (DbEntry<DbDoc, DbKw> entry : db) {
        this->update(entry);
    }
}

template <IMainDbDoc_ DbDoc, class DbKw, template<class ...> class Underly> requires IUnderly_<Underly, DbDoc, DbKw>
void SdaBase<DbDoc, DbKw, Underly>::update(const DbEntry<DbDoc, DbKw>& newEntry) {
    // if empty, initialize first index
    if (this->underlys.empty()) {
        Underly<DbDoc, DbKw> underly;
        underly.setup(this->secParam, Db<DbDoc, DbKw> {newEntry});
        this->underlys.push_back(underly);
        this->firstEmptyInd = 1;
        return;
    }

    // merge all EDB_<j into EDB_j where j is `this->firstEmptyInd`; always merge/insert into first index if it's empty
    Db<DbDoc, DbKw> mergedDb;
    for (int i = 0; i < (this->firstEmptyInd < 1 ? 1 : this->firstEmptyInd); i++) {
        // original paper fetches encrypted index and decrypts instead of `getDb()`
        // but that could get messy with Log-SRC-i as it has two indexes
        // instead we just store and get plaintext DB for convenience of implementation
        Db<DbDoc, DbKw> underlyDb = this->underlys[i].getDb();
        mergedDb.insert(mergedDb.begin(), underlyDb.begin(), underlyDb.end());
    }
    mergedDb.push_back(newEntry);
    if (this->firstEmptyInd < this->underlys.size() - 1) {
        this->underlys[this->firstEmptyInd].setup(this->secParam, mergedDb);
    } else {
        Underly<DbDoc, DbKw> newUnderly;
        newUnderly.setup(this->secParam, mergedDb);
        this->underlys.push_back(newUnderly);
    }

    // clear all EDB_<j by calling `setup()` with empty DB
    for (int i = 0; i < this->firstEmptyInd; i++) {
        this->underlys[i].setup(this->secParam, Db<DbDoc, DbKw> {});
    }

    // update the pointer to the first empty index
    int firstEmpty;
    for (firstEmpty = 0; firstEmpty < this->underlys.size(); firstEmpty++) {
        if (this->underlys[firstEmpty].isEmpty()) {
            break;
        }
    }
    this->firstEmptyInd = firstEmpty;
}

template <IMainDbDoc_ DbDoc, class DbKw, template<class ...> class Underly> requires IUnderly_<Underly, DbDoc, DbKw>
std::vector<DbDoc> SdaBase<DbDoc, DbKw, Underly>::searchWithoutHandlingDels(const Range<DbKw>& query) const {
    std::vector<DbDoc> allResults;
    // search through all non-empty indexes
    for (Underly<DbDoc, DbKw> underly : this->underlys) {
        if (underly.isEmpty()) {
            continue;
        }
        // don't use `underly.search()` here that filters out deleted tuples possibly prematurely
        // the cancelation tuple for a document is not guaranteed to be in same index as the inserting tuple
        // so we can't rely on underlying instances to filter out all deleted documents
        std::vector<DbDoc> results = underly.searchWithoutHandlingDels(query);
        allResults.insert(allResults.end(), results.begin(), results.end());
    }
    return allResults;
}

template <IMainDbDoc_ DbDoc, class DbKw, template<class ...> class Underly> requires IUnderly_<Underly, DbDoc, DbKw>
std::vector<DbDoc> Sda<DbDoc, DbKw, Underly>::search(const Range<DbKw>& query) const {
    return this->searchWithoutHandlingDels(query);
}

template <class DbKw, template<class ...> class Underly> requires IUnderly_<Underly, IdOp, DbKw>
std::vector<IdOp> Sda<IdOp, DbKw, Underly>::search(const Range<DbKw>& query) const {
    std::vector<IdOp> results = this->searchWithoutHandlingDels(query);
    return removeDeletedIdOps(results);
}

template class Sda<Id, Kw, PiBasResHiding>;
template class Sda<IdOp, Kw, PiBasResHiding>;
