#pragma once

#include <concepts>
#include <cstdint>
#include <iostream>
#include <unordered_set>
#include <vector>

#include "utils/db.h"
#include "utils/range.h"
#include "utils/tuple.h"
#include "utils/ustring.h"


namespace utils {


// yes, i know the template syntax is kinda cursed >_<
template <IsDbTuple DbTuple>
Ind<DbTuple> genInd(
    const Db<DbTuple>& db, bool shouldShuffleKwLists = false
);


template <IsDbTuple DbTuple>
Range<typename DbTuple::DbKwType> findDbKwBounds(const Db<DbTuple>& db);


template <IsDbTuple DbTuple>
std::unordered_set<Range<typename DbTuple::DbKwType>> getUniqDbKwRanges(const Db<DbTuple>& db);


template <IsDbTuple DbTuple>
void padDb(Db<DbTuple>& db, typename DbTuple::DbKwType& currMaxDbKw);


template <IsDbTuple DbTuple>
std::vector<DbTuple> cleanUpResults(const std::vector<DbTuple>& dbTuples);


uint64_t hashToPos(const ustring& hash);


} // namespace `utils`
