// WARNING: this is a slow experiment to run since it calls `setup()` with every search!!

#pragma once

#include <algorithm>
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


class AllVsDbSize : public IExperiment<ISse<>> {
private:
    // CONFIG
    inline static constexpr bigint RESULT_SIZE = 100;

public:
    AllVsDbSize(bigint maxDbSizeExp) :
        maxDbSizeExp(maxDbSizeExp)
    {
        bigint maxDbSize = std::pow(2, maxDbSizeExp);
        // make sure we can still run at least one setup/search by capping result size at DB size
        this->resultSize = std::min(RESULT_SIZE, maxDbSize);
    }

    void printHeader() const override {
        std::cout << std::endl;
        std::cout << "=============================== All vs. DB Size ================================"
                  << std::endl;
        std::cout << "Setup and search vs. DB size up to 2^" << this->maxDbSizeExp << std::endl;
        std::cout << "Fixed query result size " << this->resultSize << std::endl;
        std::cout << "================================================================================"
                  << std::endl;
        std::cout << std::endl << std::endl;
    }

    void run(ISse<>* sse, bool shouldBenchmark) const override {
        Benchmark::printHeader(shouldBenchmark);

        // (start `dbSizeExp` big enough for the query with `this->resultSize` results
        // to make sense)
        for (bigint dbSizeExp = std::ceil(std::log2(this->resultSize));
             dbSizeExp <= this->maxDbSizeExp; dbSizeExp++)
        {
            bigint dbSize = std::pow(2, dbSizeExp);
            Db<> db;
            createDb(db, this->resultSize, true, false);
            createDb(db, dbSize - this->resultSize, true, false, this->resultSize);
            Range<Kw> query {0, this->resultSize - 1};

            // setup
            sse->setup(utils::crypto::KEY_LEN, db);
            sse->benchmark->print(
                shouldBenchmark, "Setup", std::format("(size 2^{})", dbSizeExp)
            );

            // search
            sse->search(query);
            sse->benchmark->print(shouldBenchmark, "Search");

            sse->clear();
        }
        std::cout << std::endl;
    }

private:
    bigint maxDbSizeExp;
    bigint resultSize;
};


} // namespace `app::experiments`
