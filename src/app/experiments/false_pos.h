#pragma once

#include <cmath>
#include <iostream>

#include "config.h"

#include "app/db_factory.h"

#include "schemes/interfaces/sse.h"

#include "utils/benchmark.h"
#include "utils/crypto.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


namespace app::experiments::false_pos {


void printHeader(bigint maxDbSizeExp) {
    std::cout << std::endl;
    std::cout << "========================== False Positives Experiment =========================="
              << std::endl;
    std::cout << "Varied DB size up to 2^" << maxDbSizeExp << std::endl;
    std::cout << "High false positives query" << std::endl;
    std::cout << "================================================================================"
              << std::endl;
    std::cout << std::endl << std::endl;
}


void run(ISse<>* sse, bigint maxDbSize) {
    Benchmark::printHeader(config::SHOULD_BENCHMARK);

    for (bigint dbSizeExp = 2; dbSizeExp <= std::log2(maxDbSize); dbSizeExp++) {
        bigint dbSize = std::pow(2, dbSizeExp);

        // all but one tuple have keyword 0, and the remaining tuple has keyword `dbSize`
        // so for a 1-`dbSize` query, Log-SRC should select the root node as the SRC (assuming
        // `dbSize` >= 4), returning the entire DB with all but one tuple being false positives
        Db<> db;
        Kw kw1 = 0;
        Kw kw2 = dbSize;
        Range<Kw> kwRange1 {kw1, kw1};
        Range<Kw> kwRange2 {kw2, kw2};
        Range<Kw> query {1, dbSize};

        for (bigint i = 0; i < dbSize - 1; i++) {
            db.push_back(Tuple<> {i, kw1, Op::INS, kwRange1});
        }
        db.push_back(Tuple<> {dbSize - 1, kw2, Op::INS, kwRange2});

        // setup
        sse->setup(utils::crypto::KEY_LEN, db);

        // search
        sse->search(query);
        sse->benchmark->print(
            config::SHOULD_BENCHMARK, "Search", std::format("(false pos 2^{}-1)", dbSizeExp)
        );

        sse->clear();
    }
    std::cout << std::endl;
}


} // namespace `app::experiments::false_pos`
