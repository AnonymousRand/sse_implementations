#pragma once

#include <string>

#include "utils/types/basic_types.h"
#include "utils/types/ustring.h"


namespace utils {


// (this is not in `ustring.h` since it is less about the `ustring` itself)
ubigint hashToPos(const ustring& hash);


template <class CharType>
void padStr(std::basic_string<CharType>& str, int targetLen);


} // namespace `utils`
