#include <cmath>
#include <cstdint>
#include <format>
#include <iostream>
#include <memory>

#include "core/benchmark.h"
#include "core/config.h"
#include "core/sse_factory.h"

#include "schemes/log_src.h"
#include "schemes/log_src_i.h"
#include "schemes/log_src_i_star.h"
#include "schemes/n_log_n.h"
#include "schemes/pi_bas.h"
#include "schemes/sda.h"
#include "schemes/sse.h"

#include "utils/constants.h"
#include "utils/range.h"
#include "utils/sse_utils.h"


Db<> createDb(int64_t dbSize, bool isRandom, bool hasDeletions) {
    if (dbSize == 0) {
        return Db<> {};
    }
    Db<> db;
    std::uniform_int_distribution<int64_t> dist(0, dbSize - 1);

    Id minId = 1;
    Id maxId = dbSize - 1;
    Range<Kw> kwRangeDel {4, 4};
    db.push_back(DbEntry {Doc<> {0, 4, Op::INS, kwRangeDel}, kwRangeDel});
    if (hasDeletions) {
        // delete the document with keyword 4
        db.push_back(DbEntry {Doc<> {0, 4, Op::DEL, kwRangeDel}, kwRangeDel});
        maxId = dbSize - 2;
    }

    // add in debugging experiment docs if we have the space to
    if (maxId - minId >= 1) {
        Range<Kw> kwRangeDebug1 {3, 3};
        db.push_back(DbEntry {Doc<> {1, 3, Op::INS, kwRangeDebug1}, kwRangeDebug1});
        minId++;
    }
    if (maxId - minId >= 1) {
        Range<Kw> kwRangeDebug2 {5, 5};
        db.push_back(DbEntry {Doc<> {2, 5, Op::INS, kwRangeDebug2}, kwRangeDebug2});
        minId++;
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
void expDebug(ISse<>* sse, const Db<>& db, Range<Kw> query) {
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


void exp1(ISse<>* sse, int64_t dbSize) {
    if (dbSize == 0) {
        return;
    }
    Db<> db = createDb(dbSize, true, true);
    Benchmark::printHeader();

    // setup
    sse->setup(KEY_LEN, db);
    sse->benchmark->print("Setup");

    // search
    for (int64_t i = 0; i <= std::log2(dbSize); i++) {
        Range<Kw> query {0, (int64_t)std::pow(2, i) - 1};
        sse->search(query);
        sse->benchmark->print("Search", std::format("(size 2^{})", std::log2(query.size())));
    }
    std::cout << std::endl;

    sse->clear();
}


void exp2(ISse<>* sse, int64_t maxDbSize) {
    if (maxDbSize == 0) {
        return;
    }
    Range<Kw> query {0, 3};
    Benchmark::printHeader();

    for (int64_t i = 2; i <= std::log2(maxDbSize); i++) {
        int64_t dbSize = std::pow(2, i);
        Db<> db = createDb(dbSize, true, true);

        // setup
        sse->setup(KEY_LEN, db);
        sse->benchmark->print("Setup", std::format("(size 2^{})", std::log2(dbSize)));

        // search
        sse->search(query);
        sse->benchmark->print("Search", std::format("(size 2^{})", std::log2(dbSize)));
    }
    std::cout << std::endl;

    sse->clear();
}


void exp3(ISse<>* sse, int64_t maxDbSize) {
    if (maxDbSize == 0) {
        return;
    }
    Benchmark::printHeader();

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
        sse->benchmark->print("Setup", std::format("(size 2^{})", std::log2(dbSize)));

        // search
        Range<Kw> query {1, dbSize - 1};
        sse->search(query);
        sse->benchmark->print("Search", std::format("(size 2^{})", std::log2(dbSize)));
    }
    std::cout << std::endl;

    sse->clear();
}


int main() {
    int64_t maxDbSizeExp;
    std::cout << "Enter database size (power of 2): ";
    std::cin >> maxDbSizeExp;
    const int64_t maxDbSize = std::pow(2, maxDbSizeExp);
    std::cout << std::endl;

    std::unique_ptr<PiBas<>> piBas               = createSse<PiBas<>>(SHOULD_BENCHMARK);
    std::unique_ptr<NLogN<>> nLogN               = createSse<NLogN<>>(SHOULD_BENCHMARK);
    std::unique_ptr<LogSrc<PiBas>> logSrcPiBas   = createSse<LogSrc<PiBas>>(SHOULD_BENCHMARK);
    std::unique_ptr<LogSrc<NLogN>> logSrcNLogN   = createSse<LogSrc<NLogN>>(SHOULD_BENCHMARK);
    std::unique_ptr<LogSrcI<PiBas>> logSrcIPiBas = createSse<LogSrcI<PiBas>>(SHOULD_BENCHMARK);
    std::unique_ptr<LogSrcI<NLogN>> logSrcINLogN = createSse<LogSrcI<NLogN>>(SHOULD_BENCHMARK);
    std::unique_ptr<LogSrcIStar> logSrcIStar     = createSse<LogSrcIStar>(SHOULD_BENCHMARK);

    std::unique_ptr<Sda<PiBas<>>> sdaPiBas               = createDsse<Sda<PiBas<>>>(
        SHOULD_BENCHMARK, DSSE_USE_SHORTCUT_SETUP, DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<NLogN<>>> sdaNLogN               = createDsse<Sda<NLogN<>>>(
        SHOULD_BENCHMARK, DSSE_USE_SHORTCUT_SETUP, DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrc<PiBas>>> sdaLogSrcPiBas   = createDsse<Sda<LogSrc<PiBas>>>(
        SHOULD_BENCHMARK, DSSE_USE_SHORTCUT_SETUP, DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrc<NLogN>>> sdaLogSrcNLogN   = createDsse<Sda<LogSrc<NLogN>>>(
        SHOULD_BENCHMARK, DSSE_USE_SHORTCUT_SETUP, DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrcI<PiBas>>> sdaLogSrcIPiBas = createDsse<Sda<LogSrcI<PiBas>>>(
        SHOULD_BENCHMARK, DSSE_USE_SHORTCUT_SETUP, DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrcI<NLogN>>> sdaLogSrcINLogN = createDsse<Sda<LogSrcI<NLogN>>>(
        SHOULD_BENCHMARK, DSSE_USE_SHORTCUT_SETUP, DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrcIStar>> sdaLogSrcIStar     = createDsse<Sda<LogSrcIStar>>(
        SHOULD_BENCHMARK, DSSE_USE_SHORTCUT_SETUP, DSSE_SHOULD_BENCHMARK_UPDTS
    );

    //--------------------------------------------------------------------------
    // debugging experiment

    // (this is out here so all the schemes get the same DB (if it was randomized))
    Range<Kw> query {3, 5};
    Db<> db = createDb(maxDbSize, true, true); // adjust params at will

    std::cout << "============================= Debugging Experiment =============================" << std::endl;
    std::cout << "DB size: 2^" << maxDbSizeExp                                                      << std::endl;
    std::cout << "Query  : "   << query                                                             << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    std::cout << "================ PiBas =================" << std::endl;
    std::cout << std::endl;
    expDebug(piBas.get(), db, query);

    std::cout << "================ NLogN =================" << std::endl;
    std::cout << std::endl;
    expDebug(nLogN.get(), db, query);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl;
    std::cout << std::endl;
    expDebug(logSrcPiBas.get(), db, query);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl;
    std::cout << std::endl;
    expDebug(logSrcNLogN.get(), db, query);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl;
    std::cout << std::endl;
    expDebug(logSrcIPiBas.get(), db, query);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl;
    std::cout << std::endl;
    expDebug(logSrcINLogN.get(), db, query);

    std::cout << "============== Log-SRC-i* ==============" << std::endl;
    std::cout << std::endl;
    expDebug(logSrcIStar.get(), db, query);

    std::cout << "============== SDa[PiBas] ==============" << std::endl;
    std::cout << std::endl;
    expDebug(sdaPiBas.get(), db, query);

    std::cout << "============== SDa[NLogN] ==============" << std::endl;
    std::cout << std::endl;
    expDebug(sdaNLogN.get(), db, query);

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl;
    std::cout << std::endl;
    expDebug(sdaLogSrcPiBas.get(), db, query);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl;
    std::cout << std::endl;
    expDebug(sdaLogSrcNLogN.get(), db, query);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl;
    std::cout << std::endl;
    expDebug(sdaLogSrcIPiBas.get(), db, query);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl;
    std::cout << std::endl;
    expDebug(sdaLogSrcINLogN.get(), db, query);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl;
    std::cout << std::endl;
    expDebug(sdaLogSrcIStar.get(), db, query);

    //--------------------------------------------------------------------------
    // experiment 1

    std::cout << "================================= Experiment 1 =================================" << std::endl;
    std::cout << "DB size: 2^" << maxDbSizeExp                                                      << std::endl;
    std::cout << "Query  : varied"                                                                  << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    std::cout << "================ PiBas =================" << std::endl;
    std::cout << std::endl;
    exp1(piBas.get(), maxDbSize);

    std::cout << "================ NLogN =================" << std::endl;
    std::cout << std::endl;
    exp1(nLogN.get(), maxDbSize);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl;
    std::cout << std::endl;
    exp1(logSrcPiBas.get(), maxDbSize);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl;
    std::cout << std::endl;
    exp1(logSrcNLogN.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl;
    std::cout << std::endl;
    exp1(logSrcIPiBas.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl;
    std::cout << std::endl;
    exp1(logSrcINLogN.get(), maxDbSize);

    std::cout << "============== Log-SRC-i* ==============" << std::endl;
    std::cout << std::endl;
    exp1(logSrcIStar.get(), maxDbSize);

    std::cout << "============== SDa[PiBas] ==============" << std::endl;
    std::cout << std::endl;
    exp1(sdaPiBas.get(), maxDbSize);

    std::cout << "============== SDa[NLogN] ==============" << std::endl;
    std::cout << std::endl;
    exp1(sdaNLogN.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrcPiBas.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrcNLogN.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrcIPiBas.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrcINLogN.get(), maxDbSize);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrcIStar.get(), maxDbSize);

    //--------------------------------------------------------------------------
    // experiment 2

    std::cout << "================================= Experiment 2 =================================" << std::endl;
    std::cout << "DB size: varied, up to 2^" << maxDbSizeExp                                        << std::endl;
    std::cout << "Query  : 0-3"                                                                     << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    std::cout << "================ PiBas =================" << std::endl;
    std::cout << std::endl;
    exp2(piBas.get(), maxDbSize);

    std::cout << "================ NLogN =================" << std::endl;
    std::cout << std::endl;
    exp2(nLogN.get(), maxDbSize);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl;
    std::cout << std::endl;
    exp2(logSrcPiBas.get(), maxDbSize);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl;
    std::cout << std::endl;
    exp2(logSrcNLogN.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl;
    std::cout << std::endl;
    exp2(logSrcIPiBas.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl;
    std::cout << std::endl;
    exp2(logSrcINLogN.get(), maxDbSize);

    std::cout << "============== Log-SRC-i* ==============" << std::endl;
    std::cout << std::endl;
    exp2(logSrcIStar.get(), maxDbSize);

    std::cout << "============== SDa[PiBas] ==============" << std::endl;
    std::cout << std::endl;
    exp2(sdaPiBas.get(), maxDbSize);

    std::cout << "============== SDa[NLogN] ==============" << std::endl;
    std::cout << std::endl;
    exp2(sdaNLogN.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrcPiBas.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrcNLogN.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrcIPiBas.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrcINLogN.get(), maxDbSize);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrcIStar.get(), maxDbSize);
    
    //--------------------------------------------------------------------------
    // experiment 3

    std::cout << "================================= Experiment 3 =================================" << std::endl;
    std::cout << "DB size: 2^" << maxDbSizeExp                                                      << std::endl;
    std::cout << "Query  : high false positives"                                                    << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl;
    std::cout << std::endl;
    exp3(logSrcPiBas.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl;
    std::cout << std::endl;
    exp3(logSrcIPiBas.get(), maxDbSize);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl;
    std::cout << std::endl;
    exp3(logSrcNLogN.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl;
    std::cout << std::endl;
    exp3(logSrcINLogN.get(), maxDbSize);
}
