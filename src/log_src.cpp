#include "log_src.h"

#include "pi_bas.h"


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
LogSrc<Underly>::~LogSrc() {
    this->clear();
    if (this->underly != nullptr) {
        delete this->underly;
        this->underly = nullptr;
    }
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
void LogSrc<Underly>::setup(int secParam, const Db<Doc<>, Kw>& db) {
    this->clear();

    this->secParam = secParam;
    this->size = db.size();

    // build TDAG 1 over `Kw`s
    Range<Kw> kwBounds = findDbKwBounds(db);
    this->tdag = new TdagNode<Kw>(kwBounds);

    // replicate every document to all keyword ranges/TDAG nodes that cover it
    Db<Doc<>, Kw> dbWithReplications;
    dbWithReplications.reserve(calcTdagItemCount(db.size()));
    for (DbEntry<Doc<>, Kw> dbEntry : db) {
        Doc<> doc = dbEntry.first;
        Range<Kw> kwRange = dbEntry.second;
        std::list<Range<Kw>> ancestors = this->tdag->getLeafAncestors(kwRange);
        for (Range<Kw> ancestor : ancestors) {
            // make sure to update `DbKw` stored also in `Doc`!
            Doc<> newDoc(doc.get(), ancestor);
            dbWithReplications.push_back(std::pair {newDoc, ancestor});
        }
    }

    this->underly->setup(secParam, dbWithReplications);
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
std::vector<Doc<>> LogSrc<Underly>::search(const Range<Kw>& query, bool shouldCleanUpResults, bool isNaive) const {
    Range<Kw> src = this->tdag->findSrc(query);
    if (src == DUMMY_RANGE<Kw>()) {
        return std::vector<Doc<>> {};
    }
    return this->underly->search(src, shouldCleanUpResults, false);
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
void LogSrc<Underly>::clear() {
    // delete TDAG fully since it is reallocated with `new` in `setup()`
    if (this->tdag != nullptr) {
        delete this->tdag;
        this->tdag = nullptr;
    }
    this->underly->clear();
    this->size = 0;
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
void LogSrc<Underly>::getDb(Db<Doc<>, Kw>& ret) const {
    this->underly->getDb(ret);
}


template class LogSrc<PiBas>;
