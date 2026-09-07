#pragma once

#include <string>

#include "utils/types/ustring.h"


/**
 * makes all code blocks defined in `DEBUG()` no-op when compiled in non-debug mode,
 * thus speeding up release builds. use this for things that function as runtime assertions
 * (i.e. *should* always pass)!
 */
#ifndef NDEBUG
    #define DEBUG_ONLY(code) do {} while (false)
#else
    #define DEBUG_ONLY(code) do { code } while (false)
#endif


namespace utils::debug {


std::string ustrToHex(const ustring& str);
std::string ustrToHex(const ustring& str, int len);
std::string ustrToHex(const uchar* str, int len);


} // namespace `utils::debug`
