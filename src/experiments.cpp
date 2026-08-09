#include <cmath>
#include <cstdint>
#include <format>
#include <iostream>
#include <string>
#include <vector>

#include "core/benchmark.h"

#include "schemes/interfaces/sse.h"

#include "utils/constants.h"
#include "utils/doc.h"
#include "utils/range.h"
#include "utils/sse_utils.h"


namespace Experiments {


Db<> createDb(int64_t dbSize, bool isRandom, bool hasDeletions) {
    if (dbSize == 0) {
        return Db<> {};
    }
    Db<> db;
    std::uniform_int_distribution<int64_t> dist(0, dbSize - 1);

    Id minId = 0;
    Id maxId = dbSize - 1;
    if (hasDeletions) {
        // delete the document with keyword 4
        Range<Kw> kwRangeDel {4, 4};
        db.push_back(DbEntry {Doc<> {0, 4, Op::INS, kwRangeDel}, kwRangeDel});
        db.push_back(DbEntry {Doc<> {0, 4, Op::DEL, kwRangeDel}, kwRangeDel});
        maxId -= 2;
    }

    if (isRandom) {
        // fill the rest with random keywords
        for (Id id = minId; id <= maxId; id++) {
            Kw kw = dist(RNG);
            Range<Kw> kwRange {kw, kw};
            db.push_back(DbEntry {Doc<> {id, kw, Op::INS, kwRange}, kwRange});
        }
    } else {
        for (Id id = minId; id <= maxId; id++) {
            // make keywords and ids inversely proportional to test sorting of Log-SRC-i's index 2
            // and make them non-contiguous to test Log-SRC as well
            Kw kw = (dbSize - id) * 2;
            Range<Kw> kwRange {kw, kw};
            db.push_back(DbEntry {Doc<> {id, kw, Op::INS, kwRange}, kwRange});
        }
    }

    return db;
}


// experiment for debugging with fixed query and printed results
void debuggingExp(ISse<>* sse, const Db<>& db, Range<Kw> query) {
    // setup
    sse->setup(KEY_LEN, db);

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


void searchSizesExp(ISse<>* sse, int64_t dbSize) {
    if (dbSize == 0) {
        return;
    }
    Db<> db = createDb(dbSize, true, true);
    Benchmark::printHeader(Config::SHOULD_BENCHMARK);

    // setup
    sse->setup(KEY_LEN, db);
    sse->benchmark->print(Config::SHOULD_BENCHMARK, "Setup");

    // search
    for (int64_t i = 0; i <= std::log2(dbSize); i++) {
        Range<Kw> query {0, (int64_t)std::pow(2, i) - 1};
        sse->search(query);
        sse->benchmark->print(
            Config::SHOULD_BENCHMARK, "Search", std::format("(range size 2^{})", std::log2(query.size()))
        );
    }
    std::cout << std::endl;

    sse->clear();
}


void resultSizesExp(ISse<>* sse, int64_t dbSize) {
    if (dbSize == 0) {
        return;
    }
    // this is non-randomized to precisely control result sizes: then there is a unique entry per kw
    Db<> db = createDb(dbSize, false, false);
    sse->setup(KEY_LEN, db);
    Benchmark::printHeader(Config::SHOULD_BENCHMARK);

    for (int64_t resultSizeExp = 0; resultSizeExp <= std::log2(dbSize); resultSizeExp++) {
        int64_t resultSize = std::pow(2, resultSizeExp);
        // (referencing `createDb()`'s logic, this query should return exactly `resultSize`
        // results, not including false positives)
        Range<Kw> query {0, 2 * (resultSize - 1)};
        sse->search(query);
        sse->benchmark->print(
            Config::SHOULD_BENCHMARK, "Search", std::format("(result size 2^{})", resultSizeExp)
        );
    }
    std::cout << std::endl;

    sse->clear();
}


void setupSizesExp(ISse<>* sse, int64_t maxDbSize) {
    if (maxDbSize == 0) {
        return;
    }
    Range<Kw> query {0, 3};
    Benchmark::printHeader(Config::SHOULD_BENCHMARK);

    for (int64_t i = 2; i <= std::log2(maxDbSize); i++) {
        int64_t dbSize = std::pow(2, i);
        Db<> db = createDb(dbSize, true, true);

        // setup
        sse->setup(KEY_LEN, db);
        sse->benchmark->print(
            Config::SHOULD_BENCHMARK, "Setup", std::format("(size 2^{})", std::log2(dbSize))
        );

        // search
        sse->search(query);
        sse->benchmark->print(Config::SHOULD_BENCHMARK, "Search");
    }
    std::cout << std::endl;

    sse->clear();
}


void falsePosExp(ISse<>* sse, int64_t maxDbSize) {
    if (maxDbSize == 0) {
        return;
    }
    Benchmark::printHeader(Config::SHOULD_BENCHMARK);

    for (int64_t i = 2; i <= std::log2(maxDbSize); i++) {
        int64_t dbSize = std::pow(2, i);
        // two unique keywords, with all but one being 0 and the other being the max
        // thus all but one doc will be returned as false positives on a [1, n - 1] query (if the root node is the SRC)
        Db<> db;
        Kw kw1 = 0;
        Kw kw2 = dbSize - 1;
        Range<Kw> kwRange1 {kw1, kw1};
        Range<Kw> kwRange2 {kw2, kw2};
        for (int64_t i = 0; i < dbSize - 1; i++) {
            db.push_back(DbEntry {Doc<>(i, kw1, Op::INS, kwRange1), kwRange1});
        }
        db.push_back(DbEntry {Doc<>(dbSize - 1, kw2, Op::INS, kwRange2), kwRange2});

        // setup
        sse->setup(KEY_LEN, db);
        sse->benchmark->print(Config::SHOULD_BENCHMARK, "Setup");

        // search
        Range<Kw> query {1, dbSize - 1};
        sse->search(query);
        sse->benchmark->print(Config::SHOULD_BENCHMARK, "Search");
    }
    std::cout << std::endl;

    sse->clear();
}


void updatesExp(IDsse<>* dsse, int64_t dbSize) {
    if (dbSize == 0) {
        return;
    }
    Db<> db = createDb(dbSize, true, true);
    dsse->setup(KEY_LEN, Db<> {});
    Benchmark::printHeader(Config::SHOULD_BENCHMARK);

    // update one at a time
    for (int64_t i = 0; i < db.size(); i++) {
        DbEntry<> dbEntry = db[i];
        dsse->update(dbEntry);
        dsse->benchmark->print(Config::SHOULD_BENCHMARK, "Update", std::to_string(i));
    }
    std::cout << std::endl;

    dsse->clear();
}


} // namespace `Experiments`
