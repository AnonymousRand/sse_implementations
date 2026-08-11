#include "utils/debugging.h"

#include <iomanip>
#include <sstream>
#include <string>

#include "utils/ustring.h"


namespace utils {


std::string ustrToHex(const uchar* str, int len) {
    std::stringstream ss;
    for (int i = 0; i < len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<unsigned int>(str[i]) << " ";
    }
    return ss.str();
}


std::string ustrToHex(const ustring& str) {
    return ustrToHex(str.c_str(), str.length());
}


} // namespace `utils`
