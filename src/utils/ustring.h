#pragma once

#include <cstddef>
#include <iostream>
#include <string>

#include "utils/types.h"


using uchar   = unsigned char;
// use `ustring` instead of `uchar*` to avoid hell
using ustring = std::basic_string<uchar>;


namespace utils {


ustring toUstr(bigint n);
ustring toUstr(const std::string& s);
ustring toUstr(uchar* ucstr, int len);
std::string toStr(const ustring& ustr);
bigint fromUstr(const ustring& ustr);


} // namespace `utils`


std::ostream& operator <<(std::ostream& os, const ustring& ustr);


// provide hash function for `ustring`s to use faster hashmap-based structures,
// like `unordered_map` instead of `map`
template <>
struct std::hash<ustring> {
    inline std::size_t operator ()(const ustring& ustr) const noexcept {
        return std::hash<std::string>{}(utils::toStr(ustr));
    }
};
