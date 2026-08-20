#pragma once

#include <cmath>
#include <iostream>
#include <string>

#include "app/db_factory.h"
#include "app/experiments/i_experiment.h"

#include "schemes/interfaces/dsse.h"

#include "utils/benchmark.h"
#include "utils/crypto.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/tuple.h"


namespace app::experiments {


class UpdateVsDbSize : public IExperiment<IDsse<>> {
public:
    UpdateVsDbSize(bigint dbSizeExp) : dbSizeExp(dbSizeExp) {}

    void printHeader() const override {
        std::cout << std::endl;
        std::cout << "============================== Update vs. DB Size =============================="
                  << std::endl;
        std::cout << "Update vs. DB size up to 2^" << this->dbSizeExp << std::endl;
        std::cout << "================================================================================"
                  << std::endl;
        std::cout << std::endl << std::endl;
    }

    void run(IDsse<>* dsse, bool shouldBenchmark) const override {
        Benchmark::printHeader(shouldBenchmark);

        bigint dbSize = std::pow(2, this->dbSizeExp);
        Db<> db;
        createDb(db, dbSize, true, true);

        // setup (with empty DB, just to init keys and stuff)
        dsse->setup(utils::crypto::KEY_LEN, Db<> {});

        // updates
        for (bigint i = 0; i < dbSize; i++) {
            Tuple<> tuple = db[i];
            dsse->update(tuple);
            dsse->benchmark->print(shouldBenchmark, "Update", std::to_string(i));
        }
        dsse->benchmark->printUpdtAvgs(shouldBenchmark, "Averages");
        std::cout << std::endl;

        dsse->clear();
    }

private:
    bigint dbSizeExp;
};


} // namespace `app::experiments`
