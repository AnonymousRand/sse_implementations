#pragma once

#include <concepts>
#include <string>
#include <vector>

#include "utils/types/basic_types.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


namespace utils::misc {


template <IsDbTuple DbTuple>
std::vector<DbTuple> cleanUpResults(const std::vector<DbTuple>& dbTuples);


ubigint hashToPos(const ustring& hash);


template <class CharType>
void padStr(std::basic_string<CharType>& str, int targetLen);


} // namespace `utils::misc`
