#include "sda.h"

template <IMainDbDocDeriv DbDoc, class DbKw, template<class ...> class Undrly> requires ISseDeriv<Undrly, DbDoc, DbKw>
void Sda<DbDoc, DbKw, Undrly>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->secParam = secParam;
}

template <IMainDbDocDeriv DbDoc, class DbKw, template<class ...> class Undrly> requires ISseDeriv<Undrly, DbDoc, DbKw>
std::vector<DbDoc> Sda<DbDoc, DbKw, Undrly>::search(const Range<DbKw>& query) const {
    std::vector<DbDoc> allResults;
    for (Undrly<DbDoc, DbKw> undrly : this->undrlys) {
        if (undrly.isEmpty) {
            continue;
        }
        std::vector<DbDoc> results = undrly.search(query);
        // todo cancelation tuple not guaranteed to be in same index as inserting tuple so have to make final pass over?
        allResults.insert(allResults.end(), results.begin(), results.end());
    }
    return allResults;
}

template <>
std::vector<IdOp> Sda<IdOp, Kw, PiBasResult>::search(const Range<DbKw>& query) const {
    std::vector<DbDoc> allResults;
    for (Undrly<DbDoc, DbKw> undrly : this->undrlys) {
        if (undrly.isEmpty) {
            continue;
        }
        std::vector<DbDoc> results = undrly.search(query);
        // todo cancelation tuple not guaranteed to be in same index as inserting tuple so have to make final pass over?
        allResults.insert(allResults.end(), results.begin(), results.end());
    }
    return allResults;
}

template <IMainDbDocDeriv DbDoc, class DbKw, template<class ...> class Undrly> requires ISseDeriv<Undrly, DbDoc, DbKw>
void Sda<DbDoc, DbKw, Undrly>::update(const DbEntry<DbDoc, DbKw>& newEntry) {
    // if empty, initialize first index
    if (this->undrlys.empty()) {
        Undrly<DbDoc, DbKw> undrly; // todo does this need to be pointers?
        undrly.setup(this->secParam, Db<DbDoc, DbKw> {newEntry});
        this->undrlys.push_back(undrly);
        this->firstEmptyInd = 1;
        return;
    }

    // merge all EDB_<j into EDB_j where j is `this->firstEmptyInd`
    Db<DbDoc, DbKw> mergedDb;
    for (int i = 0; i < this->firstEmptyInd; i++) {
        Db<DbDoc, DbKw> undrlyDb = this->undrlys[i].getDb();
        mergedDb.insert(mergedDb.end(), undrlyDb.first(), undrlyDb.end());
    }
    mergedDb.insert(newEntry);
    if (this->firstEmptyInd < this->undrlys.size() - 1) {
        this->undrlys[this->firstEmptyInd].setup(this->secParam, mergedDb);
    } else {
        Undrly<DbDoc, DbKw> newUndrly;
        newUndrly.setup(this->secParam, mergedDb);
        this->undrlys.push_back(newUndrly);
    }

    // clear all EDB_<j by calling `setup()` with empty DB
    for (int i = 0; i < this->firstEmptyInd; i++) {
        this->underlys[i].setup(this->secParam, Db<DbDoc, DbKw> {});
    }

    // update the pointer to the first empty index (minimum 1 as index 0 will always have to be returned)
    int firstEmpty;
    for (firstEmpty = 0; firstEmpty < this->undrly.size(); firstEmpty++) {
        if (this->underly[firstEmpty].isEmpty) {
            break;
        }
    }
    this->firstEmptyInd = firstEmpty < 1 ? 1 : firstEmpty;
}

template class Sda<Id, Kw, PiBas>;
template class Sda<IdOp, Kw, PiBas>;
