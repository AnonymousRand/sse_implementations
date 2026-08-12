#pragma once

#include <concepts>
#include <cstdint>
#include <iostream>
#include <unordered_set>
#include <vector>

#include "utils/db.h"
#include "utils/ustring.h"


namespace utils {


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
Ind<DbKw, DbRecord> genInd(const Db<DbRecord, DbKw>& db, bool shouldShuffleKwLists = false);


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
Range<DbKw> findDbKwBounds(const Db<DbRecord, DbKw>& db);


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
std::unordered_set<Range<DbKw>> getUniqDbKwRanges(const Db<DbRecord, DbKw>& db);


template <IsDbRecord DbRecord>
void cleanUpResults(std::vector<DbRecord>& dbRecords);


uint64_t hashToPos(const ustring& hash);


} // namespace `utils`
