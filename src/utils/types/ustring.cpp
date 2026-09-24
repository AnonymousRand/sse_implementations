#include "utils/types/ustring.h"

#include <iostream>
#include <string>


namespace utils::ustr {


std::string toStr(const ustring& ustr) {
    std::string str;
    for (uchar c : ustr) {
        str += static_cast<char>(c);
    }
    return str;
}


} // namespace `utils::ustr`


std::ostream& operator <<(std::ostream& os, const ustring& ustr) {
    return os << utils::ustr::toStr(ustr);
}
