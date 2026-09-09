#pragma once

#include <cmath>
#include <iostream>
#include <vector>

#include "app/db_factory.h"
#include "app/experiments/i_experiment.h"

#include "schemes/interfaces/i_sse.h"

#include "utils/crypto.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/doc.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


namespace app::experiments {


class Debugging : public IExperiment<ISse<>> {
public:
    Debugging(int dbSizeExp) : dbSizeExp(dbSizeExp) {
        // CONFIG; adjust at will!!

        // DB and query declared as member variables so that they don't change between
        // calls to `run()`, for different SSE schemes
        createDb(this->db, std::pow(2, this->dbSizeExp), true, true);
        this->query = Range<Kw> {3, 5};
    }

    void printHeader() const override {
        std::cout << std::endl;
        std::cout << "================================== Debugging ==================================="
                  << std::endl;
        std::cout << "Fixed DB size 2^" << this->dbSizeExp << std::endl;
        std::cout << "Query " << this->query << std::endl;
        std::cout << "================================================================================"
                  << std::endl;
        std::cout << std::endl << std::endl;
    }

    // experiment for debugging with fixed query and printed results
    void run(ISse<>* sse) const override {
        // setup
        sse->setup(utils::crypto::KEY_LEN, this->db);

        // search
        std::vector<Doc> results = sse->search(this->query);
        std::vector<Doc> falsePositives;
        std::cout << "Results (id,kw,op):" << std::endl;
        for (const Doc& result : results) {
            Kw kw = result.kw;
            if (query.contains(kw)) {
                std::cout << result << " with keyword " << kw << std::endl;
            } else {
                falsePositives.push_back(result);
            }
        }
        std::cout << std::endl;

        std::cout << "False positives (id,kw,op):" << std::endl;
        for (const Doc& result : falsePositives) {
            std::cout << result << " with keyword " << result.kw << std::endl;
        }
        std::cout << std::endl;

        sse->clear();
    }

    // to free memory
    void clearDb() {
        this->db.clear();
    }

private:
    int dbSizeExp;
    Db<> db;
    Range<Kw> query;
};


} // namespace `app::experiments`
