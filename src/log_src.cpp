#include "log_src.h"

#include "pi_bas.h"


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
LogSrc<Underly>::LogSrc() : underly(new Underly<Doc, Kw>()) {}


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
LogSrc<Underly>::LogSrc(Underly<Doc, Kw>* underly) : underly(underly) {}


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
LogSrc<Underly>::~LogSrc() {
    this->clear();
    if (this->underly != nullptr) {
        delete this->underly;
        this->underly = nullptr;
    }
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
void LogSrc<Underly>::setup(int secParam, const Db<Doc, Kw>& db) {
    this->clear();

    this->db = db;
    // build TDAG 1 over `Kw`s
    Range<Kw> kwBounds = findDbKwBounds(db);
    this->tdag = new TdagNode<Kw>(kwBounds);

    // replicate every document to all keyword ranges/TDAG nodes that cover it
    Db<Doc, Kw> dbWithReplications;
    for (DbEntry<Doc, Kw> dbEntry : db) {
        Doc doc = dbEntry.first;
        Range<Kw> kwRange = dbEntry.second;
        std::list<Range<Kw>> ancestors = this->tdag->getLeafAncestors(kwRange);
        for (Range<Kw> ancestor : ancestors) {
            dbWithReplications.push_back(std::pair {doc, ancestor});
        }
    }

    this->underly->setup(secParam, dbWithReplications);
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
std::vector<Doc> LogSrc<Underly>::search(const Range<Kw>& query, bool shouldCleanUpResults, bool isNaive) const {
    Range<Kw> src = this->tdag->findSrc(query);
    if (src == DUMMY_RANGE<Kw>()) {
        return std::vector<Doc> {};
    }
    return this->underly->search(src, shouldCleanUpResults, false);
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
void LogSrc<Underly>::clear() {
    // delete TDAG fully since it is reallocated with `new` in `setup()`
    if (this->tdag != nullptr) {
        delete this->tdag;
        this->tdag = nullptr;
    }
    this->db.clear();
    this->underly->clear();
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
Db<Doc, Kw> LogSrc<Underly>::getDb() const {
    return this->db;
}


template <template <class ...> class Underly> requires IsSse<Underly<Doc, Kw>>
bool LogSrc<Underly>::isEmpty() const {
    return this->underly->isEmpty();
}


template class LogSrc<PiBas>;
