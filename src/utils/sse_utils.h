#pragma once

#include <concepts>
#include <cstdint>
#include <iostream>
#include <unordered_set>
#include <vector>

#include "utils/db.h"
#include "utils/ustring.h"


namespace utils {


template <class DbEntry, class DbKw> requires IsValidDbParams<DbEntry, DbKw>
Ind<DbKw, DbEntry> genInd(const Db<DbEntry, DbKw>& db, bool shouldShuffleKwLists = false);


template <class DbEntry, class DbKw> requires IsValidDbParams<DbEntry, DbKw>
Range<DbKw> findDbKwBounds(const Db<DbEntry, DbKw>& db);


template <class DbEntry, class DbKw> requires IsValidDbParams<DbEntry, DbKw>
std::unordered_set<Range<DbKw>> getUniqDbKwRanges(const Db<DbEntry, DbKw>& db);


template <IsDbEntry DbEntry>
void cleanUpResults(std::vector<DbEntry>& dbEntries);


uint64_t hashToPos(const ustring& hash);


} // namespace `utils`
