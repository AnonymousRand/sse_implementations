// WARNING: this is a slow experiment to run since `update()` is slow!!

#pragma once

#include <cmath>
#include <iostream>
#include <string>

#include "config.h"

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
    UpdateVsDbSize(const Db<>& db) : db(db) {}

    void printHeader() const override {
        std::cout << std::endl;
        std::cout << "============================== Update vs. DB Size =============================="
                  << std::endl;
        std::cout << "Update vs. DB size up to 2^" << std::ceil(std::log2(this->db.getSize()))
                  << std::endl;
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
            if (config::SHOULD_PRINT_EACH_UPDT) {
                utils::benchmark::print("Update", std::to_string(i));
            }
        }
        utils::benchmark::printUpdtAvgs("Averages");
        std::cout << std::endl;

        dsse->clear();
    }

private:
    const Db<>& db;
};


} // namespace `app::experiments`
