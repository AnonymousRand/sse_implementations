#include "sse_utils.h"

#include <algorithm>

#include "utils/doc.h"
#include "utils/range.h"
#include "utils/ustring.h"


//------------------------------------------------------------------------------
// util functions
//------------------------------------------------------------------------------


template <class IndKey, IsDbDoc DbDoc>
void shuffleInd(Ind<IndKey, DbDoc>& ind) {
    for (std::pair entry : ind) {
        std::vector<DbDoc> dbKwList = entry.second;
        std::shuffle(dbKwList.begin(), dbKwList.end(), RNG);
    }
}


template <IsDbDoc DbDoc, class DbKw>
Range<DbKw> findDbKwBounds(const Db<DbDoc, DbKw>& db) {
    if (db.empty()) {
        return DUMMY_RANGE<DbKw>();
    }

    DbKw minDbKw = DUMMY;
    DbKw maxDbKw = DUMMY;
    for (DbEntry<DbDoc, DbKw> entry : db) {
        Range<DbKw> dbKwRange = entry.second;
        if (dbKwRange.first < minDbKw || minDbKw == DUMMY) {
            minDbKw = dbKwRange.first;
        }
        if (dbKwRange.second > maxDbKw || maxDbKw == DUMMY) {
            maxDbKw = dbKwRange.second;
        }
    }
    return Range {minDbKw, maxDbKw};
}


template <IsDbDoc DbDoc, class DbKw>
std::unordered_set<Range<DbKw>> getUniqDbKwRanges(const Db<DbDoc, DbKw>& db) {
    std::unordered_set<Range<DbKw>> uniqDbKwRanges;
    for (DbEntry<DbDoc, DbKw> entry : db) {
        Range<DbKw> dbKwRange = entry.second;
        uniqDbKwRanges.insert(dbKwRange); // `unordered_set` will not insert duplicate elements
    }
    return uniqDbKwRanges;
}


template <IsDbDoc DbDoc>
void cleanUpResults(std::vector<DbDoc>& docs) {}


// template specialize just this method for `Doc<>` instead of all SSE classes that use it
template <>
void cleanUpResults(std::vector<Doc<>>& docs) {
    std::vector<Doc<>> newDocs;
    std::unordered_set<Id> deletedIds;

    // find all cancellation tuples
    for (Doc<> doc : docs) {
        Op op = doc.getOp();
        if (op == Op::DEL) {
            Id id = doc.getId();
            deletedIds.insert(id);
        }
    }
    // copy over vector without deleted (or dummy) docs, as well as no dummy ids
    for (Doc<> doc : docs) {
        Id id = doc.getId();
        Op op = doc.getOp();
        if (id != DUMMY && op == Op::INS && deletedIds.count(id) == 0) {
            newDocs.push_back(doc);
        }
    }

    docs = newDocs;
}


// TODO move this to tdag.h? (static method)
int64_t calcTdagItemCount(int64_t leafCount) {
    // see `EncIndLoc::map()` comment in `utils/enc_ind.cpp` for where this formula comes from
    int64_t topLevelNum = std::log2(leafCount);
    return topLevelNum * (2 * leafCount)
            - (1 - std::pow(2, -topLevelNum)) * std::pow(2, topLevelNum + 1)
            + leafCount;
}


uint64_t hashToPos(const ustring& hash) {
    // this conversion mess is from USENIX'24
    return (*((uint64_t*)hash.c_str()));
}


template void shuffleInd(Ind<Kw, Doc<>>& ind);
template void shuffleInd(Ind<Kw, SrcIDb1Doc>& ind);
//template void shuffleInd(Ind<IdAlias, Doc<IdAlias>>& ind);

template Range<Kw> findDbKwBounds(const Db<Doc<>, Kw>& db);
template Range<Kw> findDbKwBounds(const Db<SrcIDb1Doc, Kw>& db);
//template Range<Kw> findDbKwBounds(const Db<Doc<IdAlias>, IdAlias>& db);

template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<Doc<>, Kw>& db);
template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<SrcIDb1Doc, Kw>& db);
//template std::unordered_set<Range<IdAlias>> getUniqDbKwRanges(const Db<Doc<IdAlias>, IdAlias>& db);

template void cleanUpResults(std::vector<SrcIDb1Doc>& docs);
