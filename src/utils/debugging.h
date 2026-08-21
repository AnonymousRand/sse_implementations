#pragma once

#include <string>

#include "utils/types/ustring.h"


namespace utils::debugging {


std::string ustrToHex(const ustring& str);
std::string ustrToHex(const ustring& str, int len);
std::string ustrToHex(const uchar* str, int len);


} // namespace `utils::debugging`
