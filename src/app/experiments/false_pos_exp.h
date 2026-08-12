#pragma once

#include <cmath>
#include <cstdint>
#include <iostream>

#include "config.h"

#include "app/db_factory.h"

#include "schemes/interfaces/sse.h"

#include "utils/benchmark.h"
#include "utils/crypto.h"
#include "utils/db.h"
#include "utils/range.h"
#include "utils/types.h"


namespace app::experiments {


void falsePosExp(ISse<>* sse, int64_t maxDbSize) {
    if (maxDbSize == 0) {
        return;
    }
    Benchmark::printHeader(config::SHOULD_BENCHMARK);

    for (int64_t i = 2; i <= std::log2(maxDbSize); i++) {
        int64_t dbSize = std::pow(2, i);

        // two unique keywords, with half being 0 and the other half being the max
        // thus using Log-SRC, half the tuples will be returned as false positives on a
        // [1, n - 1] query (if the root node is the SRC)
        Db<> db;
        Kw kw1 = 0;
        Kw kw2 = dbSize - 1;
        Range<Kw> kwRange1 {kw1, kw1};
        int64_t id;
        Range<Kw> kwRange2 {kw2, kw2};
        for (id = 0; id < dbSize / 2; id++) {
            db.push_back(DbTuple {id, kw1, Op::INS, kwRange1});
        }
        for (; id < dbSize; id++) {
            db.push_back(DbTuple {id, kw2, Op::INS, kwRange2});
        }

        /*
        // two unique keywords, with all but one being 0 and the other being the max
        // thus all but one tuple will be returned as false positives on a [1, n - 1] query
        // (if the root node is the SRC)
        Db<> db;
        Kw kw1 = 0;
        Kw kw2 = dbSize - 1;
        Range<Kw> kwRange1 {kw1, kw1};
        Range<Kw> kwRange2 {kw2, kw2};
        for (int64_t i = 0; i < dbSize - 1; i++) {
            db.push_back(DbTuple {i, kw1, Op::INS, kwRange1});
        }
        db.push_back(DbTuple {dbSize - 1, kw2, Op::INS, kwRange2});
        */

        // setup
        sse->setup(utils::KEY_LEN, db);
        sse->benchmark->print(
            config::SHOULD_BENCHMARK, "Setup", std::format("(size 2^{})", std::log2(dbSize))
        );

        // search
        Range<Kw> query {1, dbSize - 1};
        sse->search(query);
        sse->benchmark->print(config::SHOULD_BENCHMARK, "Search");
    }
    std::cout << std::endl;

    sse->clear();
}


} // namespace `app::experiments`
