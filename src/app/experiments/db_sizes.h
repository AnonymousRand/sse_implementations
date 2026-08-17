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


namespace app::experiments::db_sizes {


void printHeader(bigint maxDbSizeExp) {
    std::cout << std::endl;
    std::cout << "============================= DB Sizes Experiment =============================="
              << std::endl;
    std::cout << "Varied DB size up to 2^" << maxDbSizeExp << std::endl;
    std::cout << "Fixed query 0-3" << std::endl;
    std::cout << "================================================================================"
              << std::endl;
    std::cout << std::endl << std::endl;
}


void run(ISse<>* sse, bigint maxDbSize) {
    Range<Kw> query {0, 3};
    Benchmark::printHeader(config::SHOULD_BENCHMARK);

    // (start `dbSizeExp` at 2 as otherwise the query doesn't really make sense)
    for (bigint dbSizeExp = 2; dbSizeExp <= std::log2(maxDbSize); dbSizeExp++) {
        bigint dbSize = std::pow(2, dbSizeExp);
        Db<> db = createDb(dbSize, true, true);

        // setup
        sse->setup(utils::crypto::KEY_LEN, db);
        sse->benchmark->print(
            config::SHOULD_BENCHMARK, "Setup", std::format("(size 2^{})", dbSizeExp)
        );

        // search
        sse->search(query);
        sse->benchmark->print(config::SHOULD_BENCHMARK, "Search");

        sse->clear();
    }
    std::cout << std::endl;
}


} // namespace `app::experiments::db_sizes`
