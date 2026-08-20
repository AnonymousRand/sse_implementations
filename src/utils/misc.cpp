#include "utils/misc.h"

#include <concepts>
#include <string>
#include <unordered_set>
#include <vector>

#include "utils/types/basic_types.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


namespace utils::misc {


// (we need the general case of this function to be able to call it from within the general context
// of `IStaticPointSse`; it just does nothing except in the template specialization below)
template <IsDbTuple DbTuple>
std::vector<DbTuple> cleanUpResults(const std::vector<DbTuple>& dbTuples) {
    return dbTuples;
}


// template specialize this method for just `Tuple<>` instead of all
// SSE classes that use it
template <>
std::vector<Tuple<>> cleanUpResults(const std::vector<Tuple<>>& tuples) {
    std::vector<Tuple<>> newTuples;
    std::unordered_set<Id> deletedIds;

    // find all cancellation tuples
    for (const Tuple<>& tuple : tuples) {
        Op op = tuple.getOp();
        if (op == Op::DEL) {
            deletedIds.emplace(tuple.getId());
        }
    }
    // copy over vector without deleted (or dummy) tuples, as well as no dummy ids
    for (const Tuple<>& tuple : tuples) {
        Id id = tuple.getId();
        Op op = tuple.getOp();
        if (id != DUMMY && op == Op::INS && deletedIds.count(id) == 0) {
            newTuples.push_back(tuple);
        }
    }

    return newTuples;
}


ubigint hashToPos(const ustring& hash) {
    // this conversion mess is from USENIX'24
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


//------------------------------------------------------------------------------
// explicit template instantiations


// remaining explicit template specializations beyond the one earlier
template std::vector<SrcIDb1Tuple> cleanUpResults(const std::vector<SrcIDb1Tuple>& tuples);
//template std::vector<Tuple<IdAlias>> cleanUpResults(const std::vector<Tuple<IdAlias>>& tuples);


template void padStr(std::basic_string<char>& str, bigint targetLen);
template void padStr(std::basic_string<uchar>& str, bigint targetLen);


template void unpadStr(std::basic_string<char>& str);
template void unpadStr(std::basic_string<uchar>& str);


} // namespace `utils::misc`
