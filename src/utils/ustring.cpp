#include "utils/ustring.h"

#include <iostream>
#include <string>

#include "utils/types.h"


namespace utils {


ustring toUstr(bigint n) {
    std::string str = std::to_string(n);
    return ustring(str.begin(), str.end());
}


ustring toUstr(const std::string& s) {
    return reinterpret_cast<const uchar*>(s.c_str());
}


ustring toUstr(uchar* ucstr, int len) {
    return ustring(ucstr, len);
}


std::string toStr(const ustring& ustr) {
    std::string str;
    for (uchar c : ustr) {
        str += static_cast<char>(c);
    }
    return str;
}


bigint fromUstr(const ustring& ustr) {
    return std::stol(utils::toStr(ustr));
}


} // namespace `utils`


std::ostream& operator <<(std::ostream& os, const ustring& ustr) {
    return os << utils::toStr(ustr);
}
