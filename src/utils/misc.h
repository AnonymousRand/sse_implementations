#pragma once

#include <cstdint>
#include <string>

#include "utils/ustring.h"


namespace utils {


uint64_t hashToPos(const ustring& hash);


template <class CharType>
void padStr(std::basic_string<CharType>& str, int targetLen);


} // namespace `utils`
