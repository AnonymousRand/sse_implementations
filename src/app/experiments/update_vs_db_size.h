#pragma once

#include <cmath>
#include <iostream>
#include <string>

#include "app/db_factory.h"
#include "app/experiments/i_experiment.h"

#include "schemes/interfaces/dsse.h"

#include "types/basic_types.h"
#include "types/db/db.h"
#include "types/tuple.h"

#include "utils/benchmark.h"
#include "utils/crypto.h"


namespace app::experiments {


class UpdateVsDbSize : public IExperiment<IDsse<>> {
public:
    UpdateVsDbSize(bigint dbSizeExp) : dbSizeExp(dbSizeExp) {
        // CONFIG; adjust at will!

        // DB and declared as a member variable so that it doesn't change between
        // calls to `run()`, for different SSE schemes
        createDb(this->db, std::pow(2, dbSizeExp), true, true);
    }

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
        utils::benchmark::printHeader(shouldBenchmark);

        // setup (with empty DB, just to init keys and stuff)
        dsse->setup(utils::crypto::KEY_LEN, Db<> {});

        // updates
        for (bigint i = 0; i < this->db.getSize(); i++) {
            Tuple<> tuple = this->db[i];
            dsse->update(tuple);
            utils::benchmark::print(shouldBenchmark, "Update", std::to_string(i));
        }
        utils::benchmark::printUpdtAvgs(shouldBenchmark, "Averages");
        std::cout << std::endl;

        dsse->clear();
    }

    // to free memory
    void clearDb() {
        this->db.clear();
    }

private:
    bigint dbSizeExp;
    Db<> db;
};


} // namespace `app::experiments`
