#pragma once

#include <cmath>
#include <format>
#include <iostream>

#include "app/db_factory.h"
#include "app/experiments/i_experiment.h"

#include "schemes/interfaces/i_sse.h"

#include "utils/benchmark.h"
#include "utils/crypto.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"


namespace app::experiments {


class SearchVsRangeSize : public IExperiment<ISse<>> {
public:
    SearchVsRangeSize(int dbSizeExp) : dbSizeExp(dbSizeExp) {}

    void printHeader() const override {
        std::cout << std::endl;
        std::cout << "============================ Search vs. Range Size ============================="
                  << std::endl;
        std::cout << "Search vs. query range size up to 2^" << this->dbSizeExp << std::endl;
        std::cout << "Fixed DB size 2^" << this->dbSizeExp << std::endl;
        std::cout << "================================================================================"
                  << std::endl;
        std::cout << std::endl << std::endl;
    }

    void run(ISse<>* sse) const override {
        utils::benchmark::printHeader();

        bigint dbSize = std::pow(2, this->dbSizeExp);
        Db<> db;
        createDb(db, dbSize, true, true);

        // setup
        sse->setup(utils::crypto::KEY_LEN, db);

        // searches
        for (int rangeSizeExp = 0; rangeSizeExp <= this->dbSizeExp; rangeSizeExp++) {
            Range<Kw> query {0, (Kw)std::pow(2, rangeSizeExp) - 1};
            sse->search(query);
            utils::benchmark::print("Search", std::format("(range size 2^{})", rangeSizeExp));
        }
        std::cout << std::endl;

        sse->clear();
    }

private:
    int dbSizeExp;
};


} // namespace `app::experiments`
