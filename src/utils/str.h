#pragma once

#include <string>

#include "config.h"

#include "utils/types/basic_types.h"
#include "utils/types/ustring.h"


namespace utils::str {


ustring encodeBigint(bigint sourceInt, int targetBytes = config::INT_MAX_BYTES);
bigint decodeBigint(
    const ustring& encoding, int startIndex = 0, int targetBytes = config::INT_MAX_BYTES
);


/**
 * pad `str` with '\0' bits until it has length `targetLen`.
 */
template <class CharType>
void padStrEnd(std::basic_string<CharType>& str, bigint targetLen);


ubigint hashToPos(const ustring& hash);


} // namespace `utils::str`
