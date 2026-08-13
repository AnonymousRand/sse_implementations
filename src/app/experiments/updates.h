#pragma once

#include <cstdint>
#include <iostream>
#include <string>

#include "config.h"

#include "app/db_factory.h"

#include "schemes/interfaces/dsse.h"

#include "utils/benchmark.h"
#include "utils/crypto.h"
#include "utils/types.h"


namespace app::experiments::updates {


void printHeader(int64_t maxDbSizeExp) {
    std::cout << std::endl;
    std::cout << "============================== Updates Experiment =============================="
              << std::endl;
    std::cout << "Fixed DB size 2^" << maxDbSizeExp << std::endl;
    std::cout << "One update at a time" << std::endl;
    std::cout << "================================================================================"
              << std::endl;
    std::cout << std::endl << std::endl;
}


void run(IDsse<>* dsse, int64_t dbSize) {
    if (dbSize == 0) {
        return;
    }
    Db<> db = createDb(dbSize, true, true);

    // setup (with empty DB, just to init keys and stuff)
    dsse->setup(crypto::KEY_LEN, Db<> {});
    Benchmark::printHeader(config::SHOULD_BENCHMARK);

    // updates
    for (int64_t i = 0; i < db.size(); i++) {
        Tuple<> tuple = db[i];
        dsse->update(tuple);
        dsse->benchmark->print(config::SHOULD_BENCHMARK, "Update", std::to_string(i));
    }
    std::cout << std::endl;

    dsse->clear();
}


} // namespace `app::experiments::updates`
