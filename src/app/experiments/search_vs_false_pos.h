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
        // so, create a DB where keywords span from 0 to n := log2(DB size) - 1, and
        // where there are 2^i tuples with keyword n - i for all i>=1 such that n - i
        // is in the right half of the TDAG/keyword domain, and then one tuple with
        // keyword 0 (to finish off the exact DB size)
        //
        // then, querying 0-(n - i) (again for the same i's) should choose the root node
        // to be the SRC and hence return 2^i - 1 false positives (from the right half,
        // specifically the keywords larger than n - i). moreover the overall result size
        // will always be the whole database so we can eliminate that variable.
        //
        // IMPORTANT: this only works if `this->dbSizeExp` is even! otherwise TDAG is not symmetric
        if (this->dbSizeExp % 2 == 1) {
            std::cout << "WARNING: this experiment's numbers may be off since the DB size"
                      << "is not an even power of 2!" << std::endl;
        }
        bigint largestKw = this->dbSizeExp - 1;
        Db<> db;
        db.push_back(Tuple<> {0, 0, Op::INS, Range<Kw> {0, 0}});
        bigint currId = 1;
        for (bigint i = 1; i < this->dbSizeExp / 2; i++) {
            Kw kw = largestKw - i;
            Range<Kw> kwRange {kw, kw};
            for (bigint j = 0; j < std::pow(2, i); j++) {
                db.push_back(Tuple<> {currId, kw, Op::INS, kwRange});
                currId++;
            }
        }
        std::cout << "TMP: db size is " << db.size() << "; want " << std::pow(2, this->dbSizeExp) << std::endl;

        // setup
        sse->setup(utils::crypto::KEY_LEN, db);

        // searches
        for (bigint i = 1; i < this->dbSizeExp / 2; i++) {
            Range<Kw> query {0, largestKw - i};
            auto tmp = sse->search(query);
            sse->benchmark->print(shouldBenchmark, "Search", std::format("(false pos 2^{}-1)", i));

            std::vector<Tuple<>> falsePositives;
            for (const Tuple<>& result : tmp) {
                Kw kw = result.getKw();
                if (!query.contains(kw)) {
                    falsePositives.push_back(result);
                }
            }
            std::cout << "TMP: actual false positives " << falsePositives.size() << std::endl;
        }
        std::cout << std::endl;

        sse->clear();
    }

private:
    bigint dbSizeExp;
};


} // namespace `app::experiments`
