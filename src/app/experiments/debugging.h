#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

#include "app/db_factory.h"

#include "schemes/interfaces/sse.h"

#include "utils/crypto.h"
#include "utils/db.h"
#include "utils/range.h"
#include "utils/types.h"


namespace app::experiments::debugging {


// use a pointer so that 
const Db<>* db;
const Range<Kw> query;


void init(Db<>* db, Range<Kw> query) {
    db = db;
    query = query;
}


void printHeader(int64_t maxDbSizeExp) {
    std::cout << std::endl;
    std::cout << "============================= Debugging Experiment ============================="
              << std::endl;
    std::cout << "Fixed DB size 2^" << maxDbSizeExp << std::endl;
    std::cout << "Fixed query " << query << std::endl;
    std::cout << "================================================================================"
              << std::endl;
    std::cout << std::endl << std::endl;
}


// experiment for debugging with fixed query and printed results
void run(ISse<>* sse) {
    // setup
    sse->setup(utils::KEY_LEN, *db);

    // search
    std::vector<Tuple<>> results = sse->search(query);
    std::vector<Tuple<>> falsePositives;
    std::cout << "Results ((id,kw,op),kwrange):" << std::endl;
    for (Tuple<> result : results) {
        Kw kw = result.getKw();
        if (query.contains(kw)) {
            std::cout << result << " with keyword " << kw << std::endl;
        } else {
            falsePositives.push_back(result);
        }
    }
    std::cout << std::endl;
    std::cout << "False positives ((id,kw,op),kwrange):" << std::endl;
    for (Tuple<> result : falsePositives) {
        std::cout << result << " with keyword " << result.getKw() << std::endl;
    }
    std::cout << std::endl;

    sse->clear();
}


} // namespace `app::experiments::debugging`
