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
    SearchVsFalsePos(bigint dbSizeExp) : dbSizeExp(dbSizeExp) {}

    void printHeader() const override {
        std::cout << std::endl;
        std::cout << "========================== Search vs. False Positives =========================="
                  << std::endl;
        std::cout << "Search vs. false positives up to 2^" << this->dbSizeExp << " - 1"
                  << std::endl;
        std::cout << "Fixed DB size and query result size, both 2^" << this->dbSizeExp << std::endl;
        std::cout << "================================================================================"
                  << std::endl;
        std::cout << std::endl << std::endl;
    }

    void run(ISse<>* sse, bool shouldBenchmark) const override {
        Benchmark::printHeader(shouldBenchmark);

        // for Log-SRC, the SRC will always be the root node if the leftmost keyword
        // (e.g. 0) along with anything in the right half of the TDAG is queried
        // 
        // so, create a DB where keywords span from 0 to n := 2 * log2(DB size) - 1,
        // and where there are 2^i tuples with keyword n - i for all i such that n - i
        // is in the right half of the size 2 * log2(DB size) TDAG/keyword domain,
        // and then one tuple with keyword 0 (to finish off the exact DB size)
        //
        // then, querying 0-(n - i) (again for the same i's) should choose the root node
        // to be the SRC and hence return 2^i - 1 false positives (from the right half,
        // specifically the keywords larger than n - i). moreover the overall result size
        // will always be the whole database so we can eliminate that variable
        bigint largestKw = 2 * this->dbSizeExp - 1;
        bigint currId = 0;
        Db<> db;
        db.push_back(Tuple<> {currId, 0, Op::INS, Range<Kw> {0, 0}});
        currId++;
        for (bigint i = 0; i < this->dbSizeExp; i++) {
            Kw kw = largestKw - i;
            Range<Kw> kwRange {kw, kw};
            for (bigint j = 0; j < std::pow(2, i); j++) {
                db.push_back(Tuple<> {currId, kw, Op::INS, kwRange});
                currId++;
            }
        }

        // setup
        sse->setup(utils::crypto::KEY_LEN, db);

        // searches
        for (bigint i = 0; i < this->dbSizeExp; i++) {
            Range<Kw> query {0, largestKw - i};
            sse->search(query);
            sse->benchmark->print(shouldBenchmark, "Search", std::format("(false pos 2^{}-1)", i));
        }
        std::cout << std::endl;

        sse->clear();
    }

private:
    bigint dbSizeExp;
};


} // namespace `app::experiments`
