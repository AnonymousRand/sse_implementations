#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>

#include "config.h"

#include "app/db_factory.h"

#include "schemes/interfaces/dsse.h"

#include "utils/benchmark.h"
#include "utils/cryptography.h"
#include "utils/sse_utils.h"


namespace app::experiments {


void updatesExp(IDsse<>* dsse, int64_t dbSize) {
    if (dbSize == 0) {
        return;
    }
    Db<> db = createDb(dbSize, true, true);
    dsse->setup(constants::KEY_LEN, Db<> {});
    Benchmark::printHeader(config::SHOULD_BENCHMARK);

    // update one at a time
    for (int64_t i = 0; i < db.size(); i++) {
        DbEntry<> dbEntry = db[i];
        dsse->update(dbEntry);
        dsse->benchmark->print(config::SHOULD_BENCHMARK, "Update", std::to_string(i));
    }
    std::cout << std::endl;

    dsse->clear();
}


} // namespace `app::experiments`
