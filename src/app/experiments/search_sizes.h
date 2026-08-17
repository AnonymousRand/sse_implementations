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


namespace app::experiments::search_sizes {


void printHeader(bigint maxDbSizeExp) {
    std::cout << std::endl;
    std::cout << "=========================== Search Sizes Experiment ============================"
              << std::endl;
    std::cout << "Fixed DB size 2^" << maxDbSizeExp << std::endl;
    std::cout << "Varied query range size" << std::endl;
    std::cout << "================================================================================"
              << std::endl;
    std::cout << std::endl << std::endl;
}


void run(ISse<>* sse, bigint dbSize) {
    if (dbSize == 0) {
        return;
    }
    Db<> db = createDb(dbSize, true, true);
    Benchmark::printHeader(config::SHOULD_BENCHMARK);

    // setup
    sse->setup(utils::crypto::KEY_LEN, db);
    sse->benchmark->print(config::SHOULD_BENCHMARK, "Setup");

    // searches
    for (bigint i = 0; i <= std::log2(dbSize); i++) {
        Range<Kw> query {0, (bigint)std::pow(2, i) - 1};
        sse->search(query);
        sse->benchmark->print(
            config::SHOULD_BENCHMARK, "Search",
            std::format("(range size 2^{})", std::log2(query.size()))
        );
    }
    std::cout << std::endl;

    sse->clear();
}


} // namespace `app::experiments::search_sizes`
