#include "log_src.h"


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
LogSrc<Underly>::LogSrc(EncIndType encIndType) : LogSrc(Underly<Doc, Kw>(), encIndType) {}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
LogSrc<Underly>::LogSrc(const Underly<Doc, Kw>& underly, EncIndType encIndType) : underly(underly) {
    this->setEncIndType(encIndType);
}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
LogSrc<Underly>::~LogSrc() {
    this->clear();
}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
void LogSrc<Underly>::setup(int secParam, const Db<Doc, Kw>& db) {
    this->db = db;
    // so we don't leak the memory from the previous TDAG after we call `new` again
    if (this->tdag != nullptr) {
        delete this->tdag;
        this->tdag = nullptr;
    }

    // need to find largest keyword: we can't pass in all the keywords raw, as leaves need to be contiguous
    Kw maxKw = findMaxDbKw(db);
    this->tdag = new TdagNode<Kw>(maxKw);
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

    this->underly.setup(secParam, dbWithReplications);
}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
std::vector<Doc> LogSrc<Underly>::search(const Range<Kw>& query) const {
    Range<Kw> src = this->tdag->findSrc(query);
    if (src == DUMMY_RANGE<Kw>()) {
        return std::vector<Doc> {};
    }
    return this->underly.search(src);
}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
std::vector<Doc> LogSrc<Underly>::searchWithoutRemovingDels(const Range<Kw>& query) const {
    return this->underly.searchWithoutRemovingDels(query);
}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
void LogSrc<Underly>::clear() {
    if (this->tdag != nullptr) {
        delete this->tdag;
        this->tdag = nullptr;
    }
    this->db.clear();
    this->underly.clear();
}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
Db<Doc, Kw> LogSrc<Underly>::getDb() const {
    return this->db;
}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
bool LogSrc<Underly>::isEmpty() const {
    return this->underly.isEmpty();
}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
void LogSrc<Underly>::setEncIndType(EncIndType encIndType) {
    this->underly.setEncIndType(encIndType);
}


template class LogSrc<PiBas>;
template class LogSrc<PiBasResHiding>;
