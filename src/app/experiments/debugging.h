#pragma once

#include <iostream>
#include <utility>
#include <vector>

#include "app/db_factory.h"
#include "app/experiments/i_experiment.h"

#include "schemes/interfaces/sse.h"

#include "utils/crypto.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


namespace app::experiments {


class Debugging : public IExperiment<ISse<>> {
public:
    Debugging(Db<>&& db, const Range<Kw>& query, bigint dbSizeExp) :
        db(std::move(db), query(query) {}

    void printHeader() const override {
        std::cout << std::endl;
        std::cout << "================================== Debugging ==================================="
                  << std::endl;
        std::cout << "Fixed DB size 2^" << this->db.size() << std::endl;
        std::cout << "Query " << this->query << std::endl;
        std::cout << "================================================================================"
                  << std::endl;
        std::cout << std::endl << std::endl;
    }

    // experiment for debugging with fixed query and printed results
    void run(ISse<>* sse, bool shouldBenchmark) const override {
        // setup
        sse->setup(utils::crypto::KEY_LEN, this->db);

        // search
        std::vector<Tuple<>> results = sse->search(this->query);
        std::vector<Tuple<>> falsePositives;
        std::cout << "Results ((id,kw,op),kwrange):" << std::endl;
        for (const Tuple<>& result : results) {
            Kw kw = result.getKw();
            if (query.contains(kw)) {
                std::cout << result << " with keyword " << kw << std::endl;
            } else {
                falsePositives.push_back(result);
            }
        }
        std::cout << std::endl;

        std::cout << "False positives ((id,kw,op),kwrange):" << std::endl;
        for (const Tuple<>& result : falsePositives) {
            std::cout << result << " with keyword " << result.getKw() << std::endl;
        }
        std::cout << std::endl;

        sse->clear();
    }

    // to free memory
    void clearDb() {
        this->db.clear();
    }

private:
    Db<> db;
    Range<Kw> query;
};


} // namespace `app::experiments`
