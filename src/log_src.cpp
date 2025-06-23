#include "log_src.h"

template <template <class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly<DbDoc, DbKw>>
LogSrc<Underly, DbDoc, DbKw>::LogSrc(EncIndType encIndType)
        : LogSrc(Underly<DbDoc, DbKw>(), encIndType) {}

template <template <class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly<DbDoc, DbKw>>
LogSrc<Underly, DbDoc, DbKw>::LogSrc(const Underly<DbDoc, DbKw>& underly, EncIndType encIndType)
        : underly(underly) {
    this->setEncIndType(encIndType);
}

template <template <class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly<DbDoc, DbKw>>
LogSrc<Underly, DbDoc, DbKw>::~LogSrc() {
    if (this->tdag != nullptr) {
        delete this->tdag;
    }
}

template <template <class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly<DbDoc, DbKw>>
void LogSrc<Underly, DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->db = db;

    // need to find largest keyword: we can't pass in all the keywords raw, as leaves need to be contiguous
    DbKw maxDbKw = findMaxDbKw(db);
    this->tdag = new TdagNode<DbKw>(maxDbKw);

    // replicate every document to all keyword ranges/TDAG nodes that cover it
    Db<DbDoc, DbKw> dbWithReplications;
    for (DbEntry<DbDoc, DbKw> dbEntry : db) {
        DbDoc dbDoc = dbEntry.first;
        Range<DbKw> dbKwRange = dbEntry.second;
        std::list<Range<DbKw>> ancestors = this->tdag->getLeafAncestors(dbKwRange);
        for (Range<DbKw> ancestor : ancestors) {
            dbWithReplications.push_back(std::pair {dbDoc, ancestor});
        }
    }

    this->underly.setup(secParam, dbWithReplications);
}

template <template <class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly<DbDoc, DbKw>>
std::vector<DbDoc> LogSrc<Underly, DbDoc, DbKw>::search(const Range<DbKw>& query) const {
    Range<DbKw> src = this->tdag->findSrc(query);
    if (src == DUMMY_RANGE<DbKw>()) {
        return std::vector<DbDoc> {};
    }
    return this->underly.search(src);
}

template <template <class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly<DbDoc, DbKw>>
std::vector<DbDoc> LogSrc<Underly, DbDoc, DbKw>::searchWithoutHandlingDels(const Range<DbKw>& query) const {
    return this->underly.searchWithoutHandlingDels(query);
}

template <template <class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly<DbDoc, DbKw>>
Db<DbDoc, DbKw> LogSrc<Underly, DbDoc, DbKw>::getDb() const {
    return this->db;
}

template <template <class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly<DbDoc, DbKw>>
bool LogSrc<Underly, DbDoc, DbKw>::isEmpty() const {
    return this->underly.isEmpty();
}

template <template <class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly<DbDoc, DbKw>>
void LogSrc<Underly, DbDoc, DbKw>::setEncIndType(EncIndType encIndType) {
    this->underly.setEncIndType(encIndType);
}

template class LogSrc<PiBas, Id, Kw>;
template class LogSrc<PiBas, IdOp, Kw>;

template class LogSrc<PiBasResHiding, Id, Kw>;
template class LogSrc<PiBasResHiding, IdOp, Kw>;
