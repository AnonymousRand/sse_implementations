#pragma once

#include <concepts>
#include <cstdint>
#include <iostream>
#include <unordered_set>
#include <vector>

#include "utils/db.h"
#include "utils/ustring.h"


namespace utils {


template <class DbTuple, class DbKw> requires IsValidDbParams<DbTuple, DbKw>
Ind<DbKw, DbTuple> genInd(const Db<DbTuple, DbKw>& db, bool shouldShuffleKwLists = false);


template <class DbTuple, class DbKw> requires IsValidDbParams<DbTuple, DbKw>
Range<DbKw> findDbKwBounds(const Db<DbTuple, DbKw>& db);


template <class DbTuple, class DbKw> requires IsValidDbParams<DbTuple, DbKw>
std::unordered_set<Range<DbKw>> getUniqDbKwRanges(const Db<DbTuple, DbKw>& db);


template <IsDbTuple DbTuple>
void cleanUpResults(std::vector<DbTuple>& dbTuples);


uint64_t hashToPos(const ustring& hash);


} // namespace `utils`
