#include "log_src.h"

#include "pi_bas.h"


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
LogSrc<Underly>::LogSrc() : underly(new Underly<Doc<>, Kw>()) {}


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

    // build TDAG 1 over `Kw`s
    Range<Kw> kwBounds = findDbKwBounds(db);
    this->tdag = new TdagNode<Kw>(kwBounds);

    // replicate every document to all keyword ranges/TDAG nodes that cover it
    Db<Doc<>, Kw> dbWithReplications;
    // see `EncIndLoc::map()` in `utils/enc_ind.cpp` for where this formula comes from
    long topLevel = std::log2(db.size());
    long newSize = topLevel * (2 * db.size()) - (1 - std::pow(2, -topLevel)) * std::pow(2, topLevel+1) + db.size();
    dbWithReplications.reserve(newSize);
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
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
Db<Doc<>, Kw> LogSrc<Underly>::getDb() const {
    return this->underly->getDb();
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
long LogSrc<Underly>::getSize() const {
    return this->underly->getSize();
}


template class LogSrc<PiBas>;
