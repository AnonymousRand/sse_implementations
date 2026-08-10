#pragma once

#include <cmath>
#include <cstdint>
#include <format>
#include <iostream>

#include "config.h"

#include "app/db_factory.h"

#include "schemes/interfaces/sse.h"

#include "utils/benchmark.h"
#include "utils/crypto.h"
#include "utils/doc.h"
#include "utils/range.h"
#include "utils/sse_utils.h"


namespace app::experiments {


void dbSizesExp(ISse<>* sse, int64_t maxDbSize) {
    if (maxDbSize == 0) {
        return;
    }
    Range<Kw> query {0, 3};
    Benchmark::printHeader(config::SHOULD_BENCHMARK);

    for (int64_t i = 2; i <= std::log2(maxDbSize); i++) {
        int64_t dbSize = std::pow(2, i);
        Db<> db = createDb(dbSize, true, true);

        // setup
        sse->setup(utils::KEY_LEN, db);
        sse->benchmark->print(
            config::SHOULD_BENCHMARK, "Setup", std::format("(size 2^{})", std::log2(dbSize))
        );

        // search
        sse->search(query);
        sse->benchmark->print(config::SHOULD_BENCHMARK, "Search");
    }
    std::cout << std::endl;

    sse->clear();
}


} // namespace `app::experiments`
