#pragma once

#include <cstdint>
#include <iostream>
#include <string>

#include "config.h"

#include "app/db_factory.h"

#include "schemes/interfaces/dsse.h"

#include "utils/benchmark.h"
#include "utils/crypto.h"
#include "utils/sse_utils.h"


namespace app::experiments {


void updatesExp(IDsse<>* dsse, int64_t dbSize) {
    if (dbSize == 0) {
        return;
    }
    Db<> db = createDb(dbSize, true, true);

    // setup (with empty DB, just to init keys and stuff)
    dsse->setup(utils::KEY_LEN, Db<> {});
    Benchmark::printHeader(config::SHOULD_BENCHMARK);

    // updates
    for (int64_t i = 0; i < db.size(); i++) {
        DbTuple<> dbTuple = db[i];
        dsse->update(dbTuple);
        dsse->benchmark->print(config::SHOULD_BENCHMARK, "Update", std::to_string(i));
    }
    std::cout << std::endl;

    dsse->clear();
}


} // namespace `app::experiments`
