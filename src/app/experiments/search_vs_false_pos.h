// WARNING: this is a slow experiment to run since it calls `setup()` with every search!!

#pragma once

#include <cmath>
#include <format>
#include <iostream>

#include "app/experiments/i_experiment.h"

#include "schemes/interfaces/sse.h"

#include "utils/benchmark.h"
#include "utils/crypto.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


namespace app::experiments {


class SearchVsFalsePos : public IExperiment<ISse<>> {
public:
    SearchVsFalsePos(bigint maxDbSizeExp) : maxDbSizeExp(maxDbSizeExp) {}

    void printHeader() const override {
        std::cout << std::endl;
        std::cout << "========================== Search vs. False Positives =========================="
                  << std::endl;
        std::cout << "Search vs. false positives up to 2^" << this->maxDbSizeExp << " - 1"
                  << std::endl;
        std::cout << "================================================================================"
                  << std::endl;
        std::cout << std::endl << std::endl;
    }

    void run(ISse<>* sse, bool shouldBenchmark) const override {
        Benchmark::printHeader(shouldBenchmark);

        // (start `dbSizeExp` at 2 as otherwise the query doesn't really make sense, and we also
        // want `dbSize` >= 4 at all times; see later comment)
        for (bigint dbSizeExp = 2; dbSizeExp <= this->maxDbSizeExp; dbSizeExp++) {
            bigint dbSize = std::pow(2, dbSizeExp);

            // create a DB where all but one tuple have keyword 0, and the remaining tuple
            // has keyword `dbSize`
            //
            // so for a 1-`dbSize` query, Log-SRC should select the root node as the SRC (assuming
            // `dbSize` >= 4), returning the entire DB with all but one tuple being false positives
            Db<> db;
            Kw kw1 = 0;
            Kw kw2 = dbSize;
            Range<Kw> kwRange1 {kw1, kw1};
            Range<Kw> kwRange2 {kw2, kw2};
            Range<Kw> query {1, dbSize};
            for (bigint i = 0; i < dbSize - 1; i++) {
                db.append(Tuple<> {i, kw1, Op::INS, kwRange1});
            }
            db.append(Tuple<> {dbSize - 1, kw2, Op::INS, kwRange2});

            // setup
            sse->setup(utils::crypto::KEY_LEN, db);

            // search
            sse->search(query);
            sse->benchmark->print(
                shouldBenchmark, "Search", std::format("(false pos 2^{}-1)", dbSizeExp)
            );

            sse->clear();
        }
        std::cout << std::endl;
    }

private:
    bigint maxDbSizeExp;
};


} // namespace `app::experiments`
