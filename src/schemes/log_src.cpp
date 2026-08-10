#include "schemes/log_src.h"

#include <concepts>
#include <list>
#include <utility>
#include <vector>

// for explicit template instantiation
#include "schemes/n_log_n.h"
#include "schemes/pi_bas.h"
#include "schemes/interfaces/sse.h"

#include "utils/doc.h"
#include "utils/range.h"
#include "utils/sse_utils.h"
#include "utils/tdag.h"


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
LogSrc<Underly>::~LogSrc() {
    this->clear();
    if (this->underly != nullptr) {
        delete this->underly;
        this->underly = nullptr;
    }
}


//------------------------------------------------------------------------------
// `ISse`


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
void LogSrc<Underly>::setup(int secParam, const Db<Doc<>, Kw>& db) {
    this->clear();

    this->secParam = secParam;
    this->size = db.size();

    // build TDAG 1 over `Kw`s
    Range<Kw> kwBounds = findDbKwBounds(db);
    this->tdag = new TdagNode<Kw>(kwBounds);

    // replicate every document to all keyword ranges/TDAG nodes that cover it
    Db<Doc<>, Kw> dbWithRepls;
    dbWithRepls.reserve(calcTdagEntryCount(db.size()));
    for (DbEntry<Doc<>, Kw> dbEntry : db) {
        Doc<> doc = dbEntry.first;
        Range<Kw> kwRange = dbEntry.second;
        std::list<Range<Kw>> ancestors = this->tdag->getLeafAncestors(kwRange);
        for (Range<Kw> ancestor : ancestors) {
            // make sure to update `DbKw` stored also in `Doc`!
            Doc<> newDoc(doc.get(), ancestor);
            dbWithRepls.push_back(std::pair {newDoc, ancestor});
        }
    }

    this->underly->setup(secParam, dbWithRepls);
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
    this->underly->clear();
    // delete TDAG fully since it is reallocated with `new` in `setup()`
    if (this->tdag != nullptr) {
        delete this->tdag;
        this->tdag = nullptr;
    }
    this->size = 0;
}


//------------------------------------------------------------------------------
// `ISdUnderly`


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
void LogSrc<Underly>::getDb(Db<Doc<>, Kw>& ret) const {
    // need to exclude replicated tuples: assume any tuples with `DbKw` range size >1 is replicated
    // (this doesn't seem to incur noticeable performance overhead with compiler optimizations)
    Db<Doc<>, Kw> retWithRepls;
    this->underly->getDb(retWithRepls);
    for (DbEntry<Doc<>, Kw> dbEntry : retWithRepls) {
        Doc<> dbDoc = dbEntry.first;
        Range<Kw> dbKwRange = dbDoc.getDbKwRange();
        if (dbKwRange.size() == 1) {
            ret.push_back(dbEntry);
        };
    }
}


template class LogSrc<PiBas>;
template class LogSrc<NLogN>;
