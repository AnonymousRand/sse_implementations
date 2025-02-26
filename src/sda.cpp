#include "sda.h"

template <IMainDbDocDeriv DbDoc, class DbKw, template<class ...> class Undrly> requires ISseDeriv<Undrly, DbDoc, DbKw>
void SdaBase<DbDoc, DbKw, Undrly>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->secParam = secParam;
    for (DbEntry<DbDoc, DbKw> entry : db) {
        this->update(entry);
    }
}

template <IMainDbDocDeriv DbDoc, class DbKw, template<class ...> class Undrly> requires ISseDeriv<Undrly, DbDoc, DbKw>
void SdaBase<DbDoc, DbKw, Undrly>::update(const DbEntry<DbDoc, DbKw>& newEntry) {
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
        Db<DbDoc, DbKw> undrlyDb = this->undrlys[i].get().getDb();
        mergedDb.insert(mergedDb.end(), undrlyDb.begin(), undrlyDb.end());
    }
    mergedDb.push_back(newEntry);
    if (this->firstEmptyInd < this->undrlys.size() - 1) {
        this->undrlys[this->firstEmptyInd].get().setup(this->secParam, mergedDb);
    } else {
        Undrly<DbDoc, DbKw> newUndrly;
        newUndrly.setup(this->secParam, mergedDb);
        this->undrlys.push_back(newUndrly);
    }

    // clear all EDB_<j by calling `setup()` with empty DB
    for (int i = 0; i < this->firstEmptyInd; i++) {
        this->undrlys[i].get().setup(this->secParam, Db<DbDoc, DbKw> {});
    }

    // update the pointer to the first empty index (minimum 1 as index 0 will always have to be returned)
    int firstEmpty;
    for (firstEmpty = 0; firstEmpty < this->undrlys.size(); firstEmpty++) {
        if (this->undrlys[firstEmpty].get().isEmpty) {
            break;
        }
    }
    this->firstEmptyInd = firstEmpty < 1 ? 1 : firstEmpty;

    // todo temp
    int c = 0;
    for (auto undrly : this->undrlys) {
        std::cout << "\nIndex " << c << ": -----------------------" << std::endl;
        auto db = undrly.getDb();
        for (auto entry : db) {
            std::cout << entry.first << ": " << entry.second << std::endl;
        }
        c++;
    }
}

template <IMainDbDocDeriv DbDoc, class DbKw, template<class ...> class Undrly> requires ISseDeriv<Undrly, DbDoc, DbKw>
std::vector<DbDoc> SdaBase<DbDoc, DbKw, Undrly>::searchBase(const Range<DbKw>& query) const {
    std::vector<DbDoc> allResults;
    // search through all non-empty indexes
    for (Undrly<DbDoc, DbKw> undrly : this->undrlys) {
        if (undrly.isEmpty) {
            continue;
        }
        std::vector<DbDoc> results = undrly.search(query);
        allResults.insert(allResults.end(), results.begin(), results.end());
    }
    return allResults;
}

template <IMainDbDocDeriv DbDoc, class DbKw, template<class ...> class Undrly> requires ISseDeriv<Undrly, DbDoc, DbKw>
std::vector<DbDoc> Sda<DbDoc, DbKw, Undrly>::search(const Range<DbKw>& query) const {
    return this->searchBase(query);
}

template <class DbKw, template<class ...> class Undrly> requires ISseDeriv<Undrly, IdOp, DbKw>
std::vector<IdOp> Sda<IdOp, DbKw, Undrly>::search(const Range<DbKw>& query) const {
    std::vector<IdOp> results = this->searchBase(query);
    // the cancelation tuple for a document is not guaranteed to be in same index as the inserting tuple
    // so we can't rely on undrlying instances to filter out all deleted documents
    return removeDeletedIdOps(results);
}

template class Sda<Id, Kw, PiBasResHiding>;
template class Sda<IdOp, Kw, PiBasResHiding>;
