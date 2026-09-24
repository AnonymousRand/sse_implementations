#include "utils/misc.h"

#include <bit>
#include <cassert>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

#include "utils/types/basic_types.h"
#include "utils/types/doc.h"
#include "utils/types/ustring.h"


namespace utils::misc {


// (we need the general case of this function to be able to call it from within the general context
// of `IStaticPointSse`; it just does nothing except in the template specialization below)
template <IsDbDoc DbDoc>
void cleanUpResults(std::vector<DbDoc>& results) {}


// template specialize this method for just `Tuple<>` (i.e. results of type `Doc`)
template <>
void cleanUpResults(std::vector<Doc>& results) {
    std::unordered_set<Id> deletedIds;

    // find all cancellation tuples
    for (const Doc& result : results) {
        Op op = result.op;
        if (op == Op::DEL) {
            deletedIds.emplace(result.id);
        }
    }

    // remove all deleted tuples and deletion tuples from `results` in-place
    std::erase_if(results, [&deletedIds](const Doc& result) {
        Id id = result.id;
        Op op = result.op;
        return id == DUMMY || op != Op::INS || deletedIds.contains(id);
    });
}


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


ubigint hashToPos(const ustring& hash) {
    // this conversion mess is from USENIX'24's implementation
    return (*((ubigint*)hash.c_str()));
}


template <class CharType>
void padStr(std::basic_string<CharType>& str, bigint targetLen) {
    if (str.length() < targetLen) {
        bigint amountToPad = targetLen - str.length();
        std::basic_string<CharType> padding(amountToPad, '\0');
        str += padding;
    }
}


template <class CharType>
void unpadStr(std::basic_string<CharType>& str) {
    bigint paddingStart;
    for (paddingStart = str.length() - 1; paddingStart >= 0; paddingStart--) {
        if (str[paddingStart] != '\0') {
            break;
        }
    }
    str.resize(paddingStart + 1); // `+ 1` to add back the first null terminator
}


bigint roundUpToPowOf2(bigint n) {
    assert(n >= 0);
    if (n != 0) {
        return std::pow(2, std::ceil(std::log2(n)));
    } else {
        return 0;
    }
}


//------------------------------------------------------------------------------
// explicit template instantiations


// remaining explicit template specializations beyond the one earlier
template void cleanUpResults(std::vector<SrcIDb1Doc>& results);


template void padStr(std::basic_string<char>& str, bigint targetLen);
template void padStr(std::basic_string<uchar>& str, bigint targetLen);


template void unpadStr(std::basic_string<char>& str);
template void unpadStr(std::basic_string<uchar>& str);


} // namespace `utils::misc`
