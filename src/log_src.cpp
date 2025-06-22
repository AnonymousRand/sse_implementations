#include "log_src.h"

template <template <class ...> class Underly, IEncInd_ EncInd, IMainDbDoc_ DbDoc, class DbKw>
        requires ISse_<Underly, EncInd, DbDoc, DbKw>
LogSrc<Underly, EncInd, DbDoc, DbKw>::LogSrc() : LogSrc(Underly<EncInd, DbDoc, DbKw>()) {}

template <template <class ...> class Underly, IEncInd_ EncInd, IMainDbDoc_ DbDoc, class DbKw>
        requires ISse_<Underly, EncInd, DbDoc, DbKw>
LogSrc<Underly, EncInd, DbDoc, DbKw>::LogSrc(const Underly<EncInd, DbDoc, DbKw>& underly)
        : underly(underly), tdag(nullptr) {}

template <template <class ...> class Underly, IEncInd_ EncInd, IMainDbDoc_ DbDoc, class DbKw>
        requires ISse_<Underly, EncInd, DbDoc, DbKw>
LogSrc<Underly, EncInd, DbDoc, DbKw>::~LogSrc() {
    if (this->tdag != nullptr) {
        delete this->tdag;
    }
}

template <template <class ...> class Underly, IEncInd_ EncInd, IMainDbDoc_ DbDoc, class DbKw>
        requires ISse_<Underly, EncInd, DbDoc, DbKw>
void LogSrc<Underly, EncInd, DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
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

template <template <class ...> class Underly, IEncInd_ EncInd, IMainDbDoc_ DbDoc, class DbKw>
        requires ISse_<Underly, EncInd, DbDoc, DbKw>
std::vector<DbDoc> LogSrc<Underly, EncInd, DbDoc, DbKw>::search(const Range<DbKw>& query) const {
    Range<DbKw> src = this->tdag->findSrc(query);
    if (src == DUMMY_RANGE<DbKw>()) {
        return std::vector<DbDoc> {};
    }
    return this->underly.search(src);
}

template <template <class ...> class Underly, IEncInd_ EncInd, IMainDbDoc_ DbDoc, class DbKw>
        requires ISse_<Underly, EncInd, DbDoc, DbKw>
std::vector<DbDoc> LogSrc<Underly, EncInd, DbDoc, DbKw>::searchWithoutHandlingDels(const Range<DbKw>& query) const {
    return this->underly.searchWithoutHandlingDels(query);
}

template <template <class ...> class Underly, IEncInd_ EncInd, IMainDbDoc_ DbDoc, class DbKw>
        requires ISse_<Underly, EncInd, DbDoc, DbKw>
Db<DbDoc, DbKw> LogSrc<Underly, EncInd, DbDoc, DbKw>::getDb() const {
    return this->db;
}

template <template <class ...> class Underly, IEncInd_ EncInd, IMainDbDoc_ DbDoc, class DbKw>
        requires ISse_<Underly, EncInd, DbDoc, DbKw>
bool LogSrc<Underly, EncInd, DbDoc, DbKw>::isEmpty() const {
    return this->underly.isEmpty();
}

template class LogSrc<PiBas, RamEncInd, Id, Kw>;
template class LogSrc<PiBas, RamEncInd, IdOp, Kw>;
template class LogSrc<PiBas, DiskEncInd, Id, Kw>;
template class LogSrc<PiBas, DiskEncInd, IdOp, Kw>;

template class LogSrc<PiBasResHiding, RamEncInd, Id, Kw>;
template class LogSrc<PiBasResHiding, RamEncInd, IdOp, Kw>;
template class LogSrc<PiBasResHiding, DiskEncInd, Id, Kw>;
template class LogSrc<PiBasResHiding, DiskEncInd, IdOp, Kw>;
