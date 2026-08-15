#pragma once

#include <string>

#include "utils/types.h"
#include "utils/ustring.h"


namespace utils {


ubigint hashToPos(const ustring& hash);


template <class CharType>
void padStr(std::basic_string<CharType>& str, int targetLen);


} // namespace `utils`
