#include <iostream>
#include <vector>

#include "app/db_factory.h"

#include "schemes/interfaces/sse.h"

#include "utils/cryptography.h"
#include "utils/doc.h"
#include "utils/range.h"
#include "utils/sse_utils.h"


namespace app::experiments {


// experiment for debugging with fixed query and printed results
void debuggingExp(ISse<>* sse, const Db<>& db, Range<Kw> query) {
    // setup
    sse->setup(constants::KEY_LEN, db);

    // search
    std::vector<Doc<>> results = sse->search(query);
    std::vector<Doc<>> falsePositives;
    std::cout << "Results ((id,kw,op),kwrange):" << std::endl;
    for (Doc<> result : results) {
        Kw kw = result.getKw();
        if (query.contains(kw)) {
            std::cout << result << " with keyword " << kw << std::endl;
        } else {
            falsePositives.push_back(result);
        }
    }
    std::cout << std::endl;
    std::cout << "False positives ((id,kw,op),kwrange):" << std::endl;
    for (Doc<> result : falsePositives) {
        std::cout << result << " with keyword " << result.getKw() << std::endl;
    }
    std::cout << std::endl;

    sse->clear();
}


} // namespace `app::experiments`
