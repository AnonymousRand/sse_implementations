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


namespace app::experiments::range_sizes {


void printHeader(bigint maxDbSizeExp) {
    std::cout << std::endl;
    std::cout << "============================ Range Sizes Experiment ============================"
              << std::endl;
    std::cout << "Fixed DB size 2^" << maxDbSizeExp << std::endl;
    std::cout << "Varied query range size" << std::endl;
    std::cout << "================================================================================"
              << std::endl;
    std::cout << std::endl << std::endl;
}


void run(ISse<>* sse, bigint dbSize) {
    Db<> db = createDb(dbSize, true, true);
    Benchmark::printHeader(config::SHOULD_BENCHMARK);

    // setup
    sse->setup(utils::crypto::KEY_LEN, db);

    // searches
    for (bigint rangeSizeExp = 0; rangeSizeExp <= std::log2(dbSize); rangeSizeExp++) {
        Range<Kw> query {0, (bigint)std::pow(2, rangeSizeExp) - 1};
        sse->search(query);
        sse->benchmark->print(
            config::SHOULD_BENCHMARK, "Search", std::format("(range size 2^{})", rangeSizeExp)
        );
    }
    std::cout << std::endl;

    sse->clear();
}


} // namespace `app::experiments::range_sizes`
