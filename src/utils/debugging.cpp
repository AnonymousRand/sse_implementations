#include "utils/debugging.h"

#include <format>
#include <string>

#include "utils/types/ustring.h"


namespace utils::debugging {


// NOTE: since this uses a `std::string`, it will currently refuse to print any '\0' bytes
std::string ustrToHex(const uchar* str, int len) {
    std::string hexStr = "";
    for (int i = 0; i < len; i++) {
        hexStr += std::format("{:02x} ", static_cast<unsigned int>(str[i]));
    }
    return hexStr;
}


std::string ustrToHex(const ustring& str) {
    return ustrToHex(str.c_str(), str.length());
}


} // namespace `utils::debugging`
