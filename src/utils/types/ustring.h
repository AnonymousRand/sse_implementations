#pragma once

#include <cstddef>
#include <iostream>
#include <string>

#include "utils/types/basic_types.h"


using uchar   = unsigned char;
// use `ustring` instead of `uchar*` to avoid C hell
using ustring = std::basic_string<uchar>;


// (this is not named `utils::ustring` to avoid naming conflicts with the `ustring` alias above)
namespace utils::ustr {


ustring toUstr(bigint n);
ustring toUstr(const std::string& s);
ustring toUstr(uchar* ucstr, int len);
std::string toStr(const ustring& ustr);
bigint fromUstr(const ustring& ustr);


} // namespace `utils::ustr`


std::ostream& operator <<(std::ostream& os, const ustring& ustr);


// provide hash function for `ustring`s to use faster hashmap-based structures,
// like `unordered_map` instead of `map`
template <>
struct std::hash<ustring> {
    inline std::size_t operator ()(const ustring& ustr) const noexcept {
        return std::hash<std::string>{}(utils::ustr::toStr(ustr));
    }
};
