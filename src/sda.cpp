#include "log_src.h"
#include "log_src_i.h"
#include "sda.h"

template <class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISdaUnderly_<Underly>
SdaBase<Underly, DbDoc, DbKw>::SdaBase(EncIndType encIndType) {
    this->setEncIndType(encIndType);
}

template <class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISdaUnderly_<Underly>
Sda<Underly, DbDoc, DbKw>::Sda(EncIndType encIndType) : SdaBase<Underly, DbDoc, DbKw>(encIndType) {}

template <class Underly, class DbKw> requires ISdaUnderly_<Underly>
Sda<Underly, IdOp, DbKw>::Sda(EncIndType encIndType) : SdaBase<Underly, IdOp, DbKw>(encIndType) {}

template <class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISdaUnderly_<Underly>
SdaBase<Underly, DbDoc, DbKw>::~SdaBase() {
    for (Underly* underly : this->underlys) {
        if (underly != nullptr) {
            delete underly;
            underly = nullptr;
        }
    }
}

template <class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISdaUnderly_<Underly>
void SdaBase<Underly, DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->secParam = secParam;
    if (db.empty()) {
        this->underlys.clear();
        return;
    }
    for (DbEntry<DbDoc, DbKw> entry : db) {
        this->update(entry);
    }
}

template <class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISdaUnderly_<Underly>
void SdaBase<Underly, DbDoc, DbKw>::update(const DbEntry<DbDoc, DbKw>& newEntry) {
    // if empty, initialize first index
    if (this->underlys.empty()) {
        Underly* newUnderly = new Underly();
        newUnderly->setEncIndType(this->encIndType);
        newUnderly->setup(this->secParam, Db<DbDoc, DbKw> {newEntry});
        this->underlys.push_back(newUnderly);
        this->firstEmptyInd = 1;
        return;
    }

    // merge all EDB_<j into EDB_j where j is `this->firstEmptyInd`; always merge/insert into first index if it's empty
    Db<DbDoc, DbKw> mergedDb;
    for (int i = 0; i < (this->firstEmptyInd < 1 ? 1 : this->firstEmptyInd); i++) {
        // original paper fetches encrypted index and decrypts instead of `getDb()`
        // but that could get messy with Log-SRC-i as it has two indexes
        // instead we just store and get plaintext DB for convenience of implementation
        Db<DbDoc, DbKw> underlyDb = this->underlys[i]->getDb();
        mergedDb.insert(mergedDb.begin(), underlyDb.begin(), underlyDb.end());
    }
    mergedDb.push_back(newEntry);
    if (this->firstEmptyInd < this->underlys.size() - 1) {
        this->underlys[this->firstEmptyInd]->setup(this->secParam, mergedDb);
    } else {
        Underly* newUnderly = new Underly();
        newUnderly->setEncIndType(this->encIndType);
        newUnderly->setup(this->secParam, mergedDb);
        this->underlys.push_back(newUnderly);
    }

    // clear all EDB_<j
    for (int i = 0; i < this->firstEmptyInd; i++) {
        this->underlys[i]->clear();
    }

    // update the pointer to the first empty index
    int firstEmpty;
    for (firstEmpty = 0; firstEmpty < this->underlys.size(); firstEmpty++) {
        if (this->underlys[firstEmpty]->isEmpty()) {
            break;
        }
    }
    this->firstEmptyInd = firstEmpty;
}

template <class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISdaUnderly_<Underly>
std::vector<DbDoc> Sda<Underly, DbDoc, DbKw>::search(const Range<DbKw>& query) const {
    return this->searchWithoutHandlingDels(query);
}

template <class Underly, class DbKw> requires ISdaUnderly_<Underly>
std::vector<IdOp> Sda<Underly, IdOp, DbKw>::search(const Range<DbKw>& query) const {
    std::vector<IdOp> results = this->searchWithoutHandlingDels(query);
    return removeDeletedIdOps(results);
}

template <class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISdaUnderly_<Underly>
std::vector<DbDoc> SdaBase<Underly, DbDoc, DbKw>::searchWithoutHandlingDels(const Range<DbKw>& query) const {
    std::vector<DbDoc> allResults;
    // search through all non-empty indexes
    for (Underly* underly : this->underlys) {
        if (underly->isEmpty()) {
            continue;
        }
        // don't use `underly->search()` here that filters out deleted tuples possibly prematurely
        // since the cancelation tuple for a document is not guaranteed to be in same index as the inserting tuple,
        // we can't rely on the individual underlying instances to filter out all deleted documents
        std::vector<DbDoc> results = underly->searchWithoutHandlingDels(query);
        allResults.insert(allResults.end(), results.begin(), results.end());
    }
    return allResults;
}

template <class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISdaUnderly_<Underly>
void SdaBase<Underly, DbDoc, DbKw>::setEncIndType(EncIndType encIndType) {
    this->encIndType = encIndType;
}

template class Sda<PiBasResHiding<Id, Kw>, Id, Kw>;
template class Sda<PiBasResHiding<IdOp, Kw>, IdOp, Kw>;

template class Sda<LogSrc<PiBasResHiding, Id, Kw>, Id, Kw>;
template class Sda<LogSrc<PiBasResHiding, IdOp, Kw>, IdOp, Kw>;

template class Sda<LogSrcI<PiBasResHiding, Id, Kw>, Id, Kw>;
template class Sda<LogSrcI<PiBasResHiding, IdOp, Kw>, IdOp, Kw>;
