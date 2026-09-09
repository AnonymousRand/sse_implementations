#pragma once

#include <concepts>
#include <string>
#include <vector>

#include "utils/types/basic_types.h"
#include "utils/types/doc.h"
#include "utils/types/ustring.h"


namespace utils::misc {


template <IsDbDoc DbDoc>
void cleanUpResults(std::vector<DbDoc>& results);


ubigint hashToPos(const ustring& hash);


/**
 * pad `str` with '\0' bits until it has length `targetLen`.
 */
template <class CharType>
void padStr(std::basic_string<CharType>& str, bigint targetLen);

/**
 * remove all trailing '\0' bits from `str` (except for one, which is the usual null terminator).
 */
template <class CharType>
void unpadStr(std::basic_string<CharType>& str);


} // namespace `utils::misc`
