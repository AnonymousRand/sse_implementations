#include <algorithm>
#include <random>

#include "log_src.h"

template <IMainDbDocDeriv DbDoc, class DbKw, template<class ...> class Undrly> requires ISseDeriv<Undrly, DbDoc, DbKw>
LogSrc<DbDoc, DbKw, Undrly>::LogSrc(Undrly<DbDoc, DbKw>& undrly) : undrly(undrly) {};

template <IMainDbDocDeriv DbDoc, class DbKw, template<class ...> class Undrly> requires ISseDeriv<Undrly, DbDoc, DbKw>
void LogSrc<DbDoc, DbKw, Undrly>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    // need to find largest keyword: we can't pass in all the keywords raw, as leaves need to be contiguous
    DbKw maxDbKw;
    if (!db.empty()) {
        Range<DbKw> firstDbKwRange = db[0].second;
        maxDbKw = firstDbKwRange.second;
        for (DbEntry<DbDoc, DbKw> entry : db) {
            Range<DbKw> dbKwRange = entry.second;
            if (dbKwRange.second > maxDbKw) {
                maxDbKw = dbKwRange.second;
            }
        }
    } else {
        maxDbKw = DbKw(0);
    }
    this->tdag = TdagNode<DbKw>::buildTdag(maxDbKw);

    // replicate every document to all keyword ranges/nodes in TDAG that cover it
    // temporarily use `unordered_map` (an inverted index) instead of `vector` to
    // easily identify which docs share the same keyword range for shuffling later
    std::unordered_map<Range<DbKw>, std::vector<DbDoc>> index;
    int temp = 0;
    for (DbEntry<DbDoc, DbKw> entry : db) {
        DbDoc dbDoc = entry.first;
        Range<DbKw> dbKwRange = entry.second;
        std::list<TdagNode<DbKw>*> ancestors = this->tdag->getLeafAncestors(dbKwRange);
        for (TdagNode<DbKw>* ancestor : ancestors) {
            Range<DbKw> ancestorDbKwRange = ancestor->getRange();
            if (index.count(ancestorDbKwRange) == 0) {
                index[ancestorDbKwRange] = std::vector {dbDoc};
            } else {
                index[ancestorDbKwRange].push_back(dbDoc);
            }
        }
    }

    // randomly permute documents associated with same keyword range/node and convert temporary `unordered_map` to `Db`
    Db<DbDoc, DbKw> processedDb;
    std::random_device rd;
    std::mt19937 rng(rd());
    for (std::pair pair : index) {
        Range<DbKw> dbKwRange = pair.first;
        std::vector<DbDoc> dbDocs = pair.second;
        std::shuffle(dbDocs.begin(), dbDocs.end(), rng);
        for (DbDoc dbDoc : dbDocs) {
            processedDb.push_back(std::pair {dbDoc, dbKwRange});
        }
    }

    this->undrly.setup(secParam, processedDb);
}

template <IMainDbDocDeriv DbDoc, class DbKw, template<class ...> class Undrly> requires ISseDeriv<Undrly, DbDoc, DbKw>
std::vector<DbDoc> LogSrc<DbDoc, DbKw, Undrly>::search(const Range<DbKw>& query) const {
    TdagNode<DbKw>* src = this->tdag->findSrc(query);
    if (src == nullptr) {
        return std::vector<DbDoc> {};
    }
    return this->undrly.search(src->getRange());
}

template <IMainDbDocDeriv DbDoc, class DbKw, template<class ...> class Undrly> requires ISseDeriv<Undrly, DbDoc, DbKw>
Db<DbDoc, DbKw> LogSrc<DbDoc, DbKw, Undrly>::getDb() const {
    return this->undrly.getDb();
}

template class LogSrc<Id, Kw, PiBas>;
template class LogSrc<IdOp, Kw, PiBas>;
