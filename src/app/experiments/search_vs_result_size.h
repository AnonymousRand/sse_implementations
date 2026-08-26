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

        // create a DB where for each i, there are 2^(i-1) tuples with random keywords
        // in the range [2^(i-1), 2^i - 1] (and for i = 0, a single tuple with keyword 0)
        //
        // then, querying 0-(2^i - 1) should produce exactly 2^i results
        bigint dbSize = std::pow(2, this->dbSizeExp);
        Db<> db;
        db.append(Tuple<> {0, 0, Op::INS, Range<Kw> {0, 0}});
        for (bigint i = 1; i <= this->dbSizeExp; i++) {
            bigint chunkSize = std::pow(2, i - 1);
            Kw minKw = std::pow(2, i - 1);
            Kw maxKw = std::pow(2, i) - 1;
            createDb(db, chunkSize, true, false, minKw, maxKw);
        }

        // setup
        sse->setup(utils::crypto::KEY_LEN, db);

        // searches
        for (bigint i = 0; i <= this->dbSizeExp; i++) {
            Range<Kw> query {0, (bigint)std::pow(2, i) - 1};
            sse->search(query);
            sse->benchmark->print(shouldBenchmark, "Search", std::format("(result size 2^{})", i));
        }
        std::cout << std::endl;

        sse->clear();
    }

private:
    bigint dbSizeExp;
};


} // namespace `app::experiments`
