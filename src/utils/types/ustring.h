#pragma once

#include <cstddef>
#include <format>
#include <iostream>
#include <string>


using uchar   = unsigned char;
// use `ustring` instead of `uchar*` to avoid C hell
using ustring = std::basic_string<uchar>;


// this is not named `utils::ustring` to avoid naming conflicts with the `ustring` alias above
namespace utils::ustr {


ustring toUstr(const std::string& s);
std::string toStr(const ustring& ustr);


inline const ustring EMPTY = toUstr("");


} // namespace `utils::ustr`


std::ostream& operator <<(std::ostream& os, const ustring& ustr);


// specialize `std::hash` for `ustring` so that they can be used as keys for `std::unordered_*`
template <>
struct std::hash<ustring> {
    inline std::size_t operator ()(const ustring& ustr) const noexcept {
        return std::hash<std::string>{}(utils::ustr::toStr(ustr));
    }
};


// specialize `std::formatter` for `ustring` so that they can be insert in `std::format()`
template <>
struct std::formatter<ustring> : std::formatter<std::string> {
    // inherit `parse()` from std::string

    auto format(const ustring& ustr, std::format_context& ctx) const {
        return std::formatter<std::string>::format(utils::ustr::toStr(ustr), ctx);
    }
};
