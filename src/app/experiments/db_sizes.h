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


void printHeader(bigint maxDbSizeExp, bigint fixedResultCount) {
    std::cout << std::endl;
    std::cout << "============================= DB Sizes Experiment =============================="
              << std::endl;
    std::cout << "Varied DB size up to 2^" << maxDbSizeExp << std::endl;
    std::cout << "Fixed result size " << fixedResultCount << std::endl;
    std::cout << "================================================================================"
              << std::endl;
    std::cout << std::endl << std::endl;
}


void run(ISse<>* sse, bigint maxDbSize, bigint fixedResultCount) {
    Benchmark::printHeader(config::SHOULD_BENCHMARK);

    // (start `dbSizeExp` at 2 as otherwise the query doesn't really make sense)
    for (bigint dbSizeExp = std::ceil(std::log2(fixedResultCount));
         dbSizeExp <= std::log2(maxDbSize); dbSizeExp++)
    {
        bigint dbSize = std::pow(2, dbSizeExp);
        Range<Kw> query {0, fixedResultCount - 1};
        Db<> db;
        createDb(db, fixedResultCount, true, true);
        createDb(db, dbSize - fixedResultCount, true, true, fixedResultCount);

        // setup
        sse->setup(utils::crypto::KEY_LEN, db);
        sse->benchmark->print(
            config::SHOULD_BENCHMARK, "Setup", std::format("(size 2^{})", dbSizeExp)
        );

        // search
        auto tmp = sse->search(query);
        sse->benchmark->print(config::SHOULD_BENCHMARK, "Search");
        std::cout << "result count is " << tmp.size() << std::endl;

        sse->clear();
    }
    std::cout << std::endl;
}


} // namespace `app::experiments::db_sizes`
