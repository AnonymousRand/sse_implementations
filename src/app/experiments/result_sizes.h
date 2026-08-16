#pragma once

#include <cmath>
#include <format>
#include <iostream>

#include "config.h"

#include "app/db_factory.h"

#include "schemes/interfaces/sse.h"

#include "utils/benchmark.h"
#include "utils/crypto.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"


namespace app::experiments::result_sizes {


void printHeader(bigint maxDbSizeExp) {
    std::cout << std::endl;
    std::cout << "=========================== Result Sizes Experiment ============================"
              << std::endl;
    std::cout << "Fixed DB size 2^" << maxDbSizeExp << std::endl;
    std::cout << "Varied query result size" << std::endl;
    std::cout << "================================================================================"
              << std::endl;
    std::cout << std::endl << std::endl;
}


void run(ISse<>* sse, bigint dbSize) {
    if (dbSize == 0) {
        return;
    }
    // this is non-randomized to precisely control result sizes: then there is a unique entry per kw
    Db<> db = createDb(dbSize, false, false);
    Benchmark::printHeader(config::SHOULD_BENCHMARK);

    // setup
    sse->setup(crypto::KEY_LEN, db);
    sse->benchmark->print(config::SHOULD_BENCHMARK, "Setup");

    // searches
    for (bigint resultSizeExp = 0; resultSizeExp <= std::log2(dbSize); resultSizeExp++) {
        bigint resultSize = std::pow(2, resultSizeExp);
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


} // namespace `app::experiments::result_sizes`
