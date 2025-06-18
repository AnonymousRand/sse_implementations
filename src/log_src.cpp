#include "log_src.h"

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
LogSrc<Underly, DbDoc, DbKw>::LogSrc() : LogSrc(Underly<DbDoc, DbKw>()) {};

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
LogSrc<Underly, DbDoc, DbKw>::LogSrc(const Underly<DbDoc, DbKw>& underly) : underly(underly) {};

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
void LogSrc<Underly, DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->db = db;

    // need to find largest keyword: we can't pass in all the keywords raw, as leaves need to be contiguous
    DbKw maxDbKw = findMaxDbKw(db);
    this->tdag = TdagNode<DbKw>::buildTdag(maxDbKw);

    // replicate every document to all keyword ranges/nodes in TDAG that cover it
    // temporarily use `unordered_map` (an inverted index) instead of `vector` to
    // easily identify which docs share the same keyword range for shuffling later
    Ind<DbKw, DbDoc> ind;
    int temp = 0;
    for (DbEntry<DbDoc, DbKw> entry : db) {
        DbDoc dbDoc = entry.first;
        Range<DbKw> dbKwRange = entry.second;
        std::list<TdagNode<DbKw>*> ancestors = this->tdag->getLeafAncestors(dbKwRange);
        for (TdagNode<DbKw>* ancestor : ancestors) {
            Range<DbKw> ancestorDbKwRange = ancestor->getRange();
            if (ind.count(ancestorDbKwRange) == 0) {
                ind[ancestorDbKwRange] = std::vector {dbDoc};
            } else {
                ind[ancestorDbKwRange].push_back(dbDoc);
            }
        }
    }

    // randomly permute documents associated with same keyword range/node and convert temporary `unordered_map` to `Db`
    // (need temporary index to facilitate shuffling)
    shuffleInd(ind);
    Db<DbDoc, DbKw> processedDb = indToDb(ind);

    this->underly.setup(secParam, processedDb);
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
std::vector<DbDoc> LogSrc<Underly, DbDoc, DbKw>::search(const Range<DbKw>& query) const {
    TdagNode<DbKw>* src = this->tdag->findSrc(query);
    if (src == nullptr) {
        return std::vector<DbDoc> {};
    }
    return this->underly.search(src->getRange());
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
std::vector<DbDoc> LogSrc<Underly, DbDoc, DbKw>::searchWithoutHandlingDels(const Range<DbKw>& query) const {
    return this->underly.searchWithoutHandlingDels(query);
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
Db<DbDoc, DbKw> LogSrc<Underly, DbDoc, DbKw>::getDb() const {
    return this->db;
}

template <template<class ...> class Underly, IMainDbDoc_ DbDoc, class DbKw> requires ISse_<Underly, DbDoc, DbKw>
bool LogSrc<Underly, DbDoc, DbKw>::isEmpty() const {
    return this->underly.isEmpty();
}

template class LogSrc<PiBas, Id, Kw>;
template class LogSrc<PiBas, IdOp, Kw>;

template class LogSrc<PiBasResHiding, Id, Kw>;
template class LogSrc<PiBasResHiding, IdOp, Kw>;
