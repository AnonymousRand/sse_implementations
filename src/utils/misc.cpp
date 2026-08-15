#include "utils/misc.h"

#include <string>

#include "utils/types.h"
#include "utils/ustring.h"


namespace utils {


ubigint hashToPos(const ustring& hash) {
    // this conversion mess is from USENIX'24
    return (*((ubigint*)hash.c_str()));
}


template <class CharType>
void padStr(std::basic_string<CharType>& str, int targetLen) {
    if (str.length() < targetLen) {
        int amountToPad = targetLen - str.length();
        std::basic_string<CharType> padding(amountToPad, '\0');
        str += padding;
    }
}


//------------------------------------------------------------------------------
// explicit template instantiations


template void padStr(std::basic_string<char>& str, int targetLen);
template void padStr(std::basic_string<uchar>& str, int targetLen);


} // namespace `utils`
