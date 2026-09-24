#include "utils/misc.h"

#include <cassert>
#include <cmath>
#include <concepts>
#include <unordered_set>
#include <vector>

#include "utils/types/basic_types.h"
#include "utils/types/doc.h"


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


} // namespace `utils::misc`
