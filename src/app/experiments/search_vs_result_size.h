#pragma once

#include <cmath>
#include <format>
#include <iostream>

#include "app/db_factory.h"
#include "app/experiments/i_experiment.h"

#include "schemes/interfaces/sse.h"

#include "utils/benchmark.h"
#include "utils/crypto.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"


namespace app::experiments {


class SearchVsResultSize : public IExperiment<ISse<>> {
public:
    SearchVsResultSize(bigint dbSizeExp) : dbSizeExp(dbSizeExp) {}

    void printHeader() const override {
        std::cout << std::endl;
        std::cout << "============================ Search vs. Result Size ============================"
                  << std::endl;
        std::cout << "Search vs. query result size up to 2^" << this->dbSizeExp << std::endl;
        std::cout << "Fixed DB size 2^" << this->dbSizeExp << std::endl;
        std::cout << "================================================================================"
                  << std::endl;
        std::cout << std::endl << std::endl;
    }

    void run(ISse<>* sse, bool shouldBenchmark) const override {
        Benchmark::printHeader(shouldBenchmark);

        bigint dbSize = std::pow(2, this->dbSizeExp);
        for (bigint resultSizeExp = 0; resultSizeExp <= this->dbSizeExp; resultSizeExp++) {
            bigint resultSize = std::pow(2, resultSizeExp);
            Db<> db;
            createDb(db, resultSize, true, false);
            createDb(db, dbSize - resultSize, true, false, resultSize);
            Range<Kw> query {0, resultSize - 1};

            // setup
            sse->setup(utils::crypto::KEY_LEN, db);

            // search
            sse->search(query);
            sse->benchmark->print(
                shouldBenchmark, "Search", std::format("(result size 2^{})", resultSizeExp)
            );

            sse->clear();
        }
        std::cout << std::endl;
    }

private:
    bigint dbSizeExp;
};


} // namespace `app::experiments`
