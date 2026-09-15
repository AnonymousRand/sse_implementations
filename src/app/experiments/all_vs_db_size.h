// WARNING: this is a slow experiment to run since it calls `setup()` with every search!!

#pragma once

#include <algorithm>
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


class AllVsDbSize : public IExperiment<ISse<>> {
public:
    AllVsDbSize(int maxDbSizeExp, bigint targetResSize) : maxDbSizeExp(maxDbSizeExp) {
        // make sure we can still run at least one setup/search by capping result size at DB size
        bigint maxDbSize = std::pow(2, maxDbSizeExp);
        this->resSize = std::min(targetResSize, maxDbSize);
    }

    void printHeader() const override {
        std::cout << std::endl;
        std::cout << "=============================== All vs. DB Size ================================"
                  << std::endl;
        std::cout << "Setup and search vs. DB size up to 2^" << this->maxDbSizeExp << std::endl;
        std::cout << "Fixed query result size " << this->resSize << std::endl;
        std::cout << "================================================================================"
                  << std::endl;
        std::cout << std::endl << std::endl;
    }

    void run(ISse<>* sse) const override {
        utils::benchmark::printHeader();

        // we start `dbSizeExp` big enough for a query with `this->resSize` results
        // to make sense
        for (int dbSizeExp = std::ceil(std::log2(this->resSize));
            dbSizeExp <= this->maxDbSizeExp; dbSizeExp++)
        {
            bigint dbSize = std::pow(2, dbSizeExp);
            Db<> db;
            // make sure only `this->resSize` tuples have the right kws to be returned as results
            createDb(db, this->resSize, true, false);
            createDb(db, dbSize - this->resSize, true, false, this->resSize);
            Range<Kw> query {0, this->resSize - 1};

            // setup
            sse->setup(utils::crypto::KEY_LEN, db);
            utils::benchmark::print("Setup", std::format("(size 2^{})", dbSizeExp));

            // search
            sse->search(query);
            utils::benchmark::print("Search");

            sse->clear();
        }
        std::cout << std::endl;
    }

private:
    int maxDbSizeExp;
    bigint resSize;
};


} // namespace `app::experiments`
