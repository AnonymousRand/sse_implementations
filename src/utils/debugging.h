#pragma once

#include <string>

#include "utils/ustring.h"


namespace utils {


std::string ustrToHex(const uchar* str, int len);


std::string ustrToHex(const ustring& str);


} // namespace `utils`
