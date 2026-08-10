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


void searchSizesExp(ISse<>* sse, int64_t dbSize) {
    if (dbSize == 0) {
        return;
    }
    Db<> db = createDb(dbSize, true, true);
    Benchmark::printHeader(config::SHOULD_BENCHMARK);

    // setup
    sse->setup(utils::KEY_LEN, db);
    sse->benchmark->print(config::SHOULD_BENCHMARK, "Setup");

    // search
    for (int64_t i = 0; i <= std::log2(dbSize); i++) {
        Range<Kw> query {0, (int64_t)std::pow(2, i) - 1};
        sse->search(query);
        sse->benchmark->print(
            config::SHOULD_BENCHMARK, "Search", std::format("(range size 2^{})", std::log2(query.size()))
        );
    }
    std::cout << std::endl;

    sse->clear();
}


} // namespace `app::experiments`
