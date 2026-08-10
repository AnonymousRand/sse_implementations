#include <cmath>
#include <cstdint>
#include <format>
#include <iostream>

#include "config.h"

#include "app/db_factory.h"

#include "schemes/interfaces/sse.h"

#include "utils/benchmark.h"
#include "utils/cryptography.h"
#include "utils/doc.h"
#include "utils/range.h"
#include "utils/sse_utils.h"


namespace app::experiments {


void resultSizesExp(ISse<>* sse, int64_t dbSize) {
    if (dbSize == 0) {
        return;
    }
    // this is non-randomized to precisely control result sizes: then there is a unique entry per kw
    Db<> db = createDb(dbSize, false, false);
    sse->setup(constants::KEY_LEN, db);
    Benchmark::printHeader(config::SHOULD_BENCHMARK);

    for (int64_t resultSizeExp = 0; resultSizeExp <= std::log2(dbSize); resultSizeExp++) {
        int64_t resultSize = std::pow(2, resultSizeExp);
        // (referencing `createDb()`'s logic, this query should return exactly `resultSize`
        // results, not including false positives)
        Range<Kw> query {0, 2 * (resultSize - 1)};
        sse->search(query);
        sse->benchmark->print(
            config::SHOULD_BENCHMARK, "Search", std::format("(result size 2^{})", resultSizeExp)
        );
    }
    std::cout << std::endl;

    sse->clear();
}


} // namespace `app::experiments`
