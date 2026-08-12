#pragma once

#include <concepts>
#include <cstdint>
#include <iostream>
#include <unordered_set>
#include <vector>

#include "utils/db.h"
#include "utils/range.h"
#include "utils/ustring.h"


namespace utils {


template <class DbTuple, class DbKw> requires IsValidDbParams<DbTuple, DbKw>
Ind<DbKw, DbTuple> genInd(const Db<DbTuple>& db, bool shouldShuffleKwLists = false);


template <class DbTuple, class DbKw> requires IsValidDbParams<DbTuple, DbKw>
Range<DbKw> findDbKwBounds(const Db<DbTuple>& db);


template <class DbTuple, class DbKw> requires IsValidDbParams<DbTuple, DbKw>
std::unordered_set<Range<DbKw>> getUniqDbKwRanges(const Db<DbTuple>& db);


template <IsDbTuple DbTuple>
void cleanUpResults(std::vector<DbTuple>& dbTuples);


uint64_t hashToPos(const ustring& hash);


} // namespace `utils`
