#include "utils/misc.h"

#include <cmath>
#include <concepts>
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
    str.resize(paddingStart + 1); // (`+ 1` to add back the first null terminator)
}


bigint roundUpToPowOf2(bigint n) {
    return std::pow(2, std::ceil(std::log2(n)));
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
