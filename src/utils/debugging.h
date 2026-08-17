#pragma once

#include <string>

#include "utils/types/ustring.h"


namespace utils::debugging {


std::string ustrToHex(const uchar* str, int len);


std::string ustrToHex(const ustring& str);


} // namespace `utils::debugging`
