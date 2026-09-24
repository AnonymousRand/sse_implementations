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


void encodeBigint(uchar* ret, bigint sourceInt, int targetBytes) {
    if constexpr (std::endian::native == std::endian::little) {
        // on little-endian systems, `std::memcpy()` already copies LSB first, which is what we want
        std::memcpy(ret, &sourceInt, targetBytes);
    } else {
        // otherwise we must copy byte by byte, with less significant bytes earlier in `ret`
        for (int i = 0; i < targetBytes; i++) {
            ret[i] = static_cast<uchar>((sourceInt >> (8 * (targetBytes - i - 1))) & 0xff);
        }
    }
}


bigint decodeBigint(const uchar* encoding, int targetBytes) {
    // init to 0 so that unfilled bytes are `0`
    bigint ret = 0;
    if constexpr (std::endian::native == std::endian::little) {
        std::memcpy(&ret, encoding, targetBytes);
    } else {
        for (int i = 0; i < targetBytes; i++) {
            // cast to `std::uint8_t` first to avoid sign extension issues when bitshifting
            std::uint8_t byte = static_cast<std::uint8_t>(encoding[i]);
            ret |= (static_cast<bigint>(byte) >> (8 * (targetBytes - i - 1)));
        }
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
