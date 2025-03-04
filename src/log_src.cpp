#include "log_src.h"

template <IMainDbDoc_ DbDoc, class DbKw, template<class ...> class Underly> requires ISse_<Underly, DbDoc, DbKw>
LogSrc<DbDoc, DbKw, Underly>::LogSrc(Underly<DbDoc, DbKw>& underly) : underly(underly) {};

template <IMainDbDoc_ DbDoc, class DbKw, template<class ...> class Underly> requires ISse_<Underly, DbDoc, DbKw>
void LogSrc<DbDoc, DbKw, Underly>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
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
    shuffleInd(ind);
    Db<DbDoc, DbKw> processedDb = indToDb(ind);

    this->underly.setup(secParam, processedDb);
}

template <IMainDbDoc_ DbDoc, class DbKw, template<class ...> class Underly> requires ISse_<Underly, DbDoc, DbKw>
std::vector<DbDoc> LogSrc<DbDoc, DbKw, Underly>::search(const Range<DbKw>& query) const {
    TdagNode<DbKw>* src = this->tdag->findSrc(query);
    if (src == nullptr) {
        return std::vector<DbDoc> {};
    }
    return this->underly.search(src->getRange());
}

template <IMainDbDoc_ DbDoc, class DbKw, template<class ...> class Underly> requires ISse_<Underly, DbDoc, DbKw>
Db<DbDoc, DbKw> LogSrc<DbDoc, DbKw, Underly>::getDb() const {
    return this->underly.getDb();
}

template <IMainDbDoc_ DbDoc, class DbKw, template<class ...> class Underly> requires ISse_<Underly, DbDoc, DbKw>
bool LogSrc<DbDoc, DbKw, Underly>::isEmpty() const {
    return this->underly.isEmpty();
}

template class LogSrc<Id, Kw, PiBas>;
template class LogSrc<IdOp, Kw, PiBas>;
