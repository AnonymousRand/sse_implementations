#include "utils/str.h"

#include <bit>
#include <cstdint>
#include <cstring>
#include <string>

#include "utils/types/basic_types.h"
#include "utils/types/ustring.h"


namespace utils::str {


ustring encodeBigint(bigint sourceInt, int targetBytes) {
    ustring ret;
    ret.resize(targetBytes);
    if constexpr (std::endian::native == std::endian::little) {
        // on little-endian systems, `std::memcpy()` already copies LSB first, which is what we want
        // note that `memcpy()`ing directly to a C++ string requires resizing first!
        std::memcpy(ret.data(), &sourceInt, targetBytes);
    } else {
        // otherwise we must copy byte by byte, with less significant bytes earlier in `ret`
        for (int i = 0; i < targetBytes; i++) {
            ret[i] = static_cast<uchar>((sourceInt >> (8 * (targetBytes - i - 1))) & 0xff);
        }
    }
    return ret;
}


bigint decodeBigint(const ustring& encoding, int startIndex, int targetBytes) {
    // init to 0 so that unfilled bytes are `0`
    bigint ret = 0;
    int msbIndex;
    if constexpr (std::endian::native == std::endian::little) {
        msbIndex = startIndex + targetBytes - 1;
        std::memcpy(&ret, encoding.c_str() + startIndex, targetBytes);
    } else {
        msbIndex = startIndex;
        for (int i = 0; i < targetBytes; i++) {
            // cast to `std::uint8_t` first to avoid sign extension issues when bitshifting
            std::uint8_t byte = static_cast<std::uint8_t>(encoding[startIndex + i]);
            ret |= (static_cast<bigint>(byte) >> (8 * (targetBytes - i - 1)));
        }
    }

    // if our encoding is not the exact length of `bigint` and the sign bit at the MSB is `1`,
    // we need to manually fill the remaining bits of the returned `bigint` that we didn't fill
    // with `1` bits in order to not read the highest possible `ubigint` for `-1`, for example
    if (targetBytes < sizeof(bigint) && (static_cast<std::uint8_t>(encoding[msbIndex]) & 0x80)) {
        ubigint allOneBits = ~ubigint(0);
        bigint mask = ~(allOneBits >> (sizeof(bigint) - targetBytes) * 8);
        ret |= mask;
    }
    return ret;
}


template <class CharType>
void padStrEnd(std::basic_string<CharType>& str, bigint targetLen) {
    if (str.length() < targetLen) {
        bigint amountToPad = targetLen - str.length();
        std::basic_string<CharType> padding(amountToPad, '\0');
        str += padding;
    }
}


template <class CharType>
void unpadStrEnd(std::basic_string<CharType>& str) {
    bigint paddingStart;
    for (paddingStart = str.length() - 1; paddingStart >= 0; paddingStart--) {
        if (str[paddingStart] != '\0') {
            break;
        }
    }
    str.resize(paddingStart + 1); // `+ 1` to add back the first null terminator
}


ubigint hashToPos(const ustring& hash) {
    // this conversion mess is from USENIX'24's implementation
    return (*((ubigint*)hash.c_str()));
}


//------------------------------------------------------------------------------
// explicit template instantiations


template void padStrEnd(std::basic_string<char>& str, bigint targetLen);
template void padStrEnd(std::basic_string<uchar>& str, bigint targetLen);


template void unpadStrEnd(std::basic_string<char>& str);
template void unpadStrEnd(std::basic_string<uchar>& str);


} // namespace `utils::str`
