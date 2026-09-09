#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

#include "app/db_factory.h"
#include "app/experiments/i_experiment.h"

#include "schemes/interfaces/i_dsse.h"

#include "utils/benchmark.h"
#include "utils/crypto.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/tuple.h"


namespace app::experiments {


class UpdateVsDbSize : public IExperiment<IDsse<>> {
public:
    UpdateVsDbSize(int dbSizeExp) {
        // CONFIG; adjust at will!

        // prevent this experiment from taking far too long and outputting far too much text
        this->dbSizeExp = std::min(dbSizeExp, 3);

        // DB and declared as a member variable so that it doesn't change between
        // calls to `run()`, for different SSE schemes
        createDb(this->db, std::pow(2, this->dbSizeExp), true, true);
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

    void run(IDsse<>* dsse) const override {
        utils::benchmark::printHeader();

        // setup (with empty DB, just to init keys and stuff)
        dsse->setup(utils::crypto::KEY_LEN, Db<> {});

        // updates
        for (bigint i = 0; i < this->db.getSize(); i++) {
            Tuple<> tuple = this->db[i];
            dsse->update(tuple);
            utils::benchmark::print("Update", std::to_string(i));
        }
        utils::benchmark::printUpdtAvgs("Averages");
        std::cout << std::endl;

        dsse->clear();
    }

    // to free memory
    void clearDb() {
        this->db.clear();
    }

private:
    int dbSizeExp;
    Db<> db;
};


} // namespace `app::experiments`
