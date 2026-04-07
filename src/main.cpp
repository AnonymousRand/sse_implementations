#include <chrono>
#include <cmath>

#include "schemes/log_src.h"
#include "schemes/log_src_i.h"
#include "schemes/log_src_i_star.h"
#include "schemes/nlogn.h"
#include "schemes/pibas.h"
#include "schemes/sda.h"


Db<> createDb(long dbSize, bool isRandom, bool hasDeletions) {
    if (dbSize == 0) {
        return Db<> {};
    }
    Db<> db;
    std::uniform_int_distribution<long> dist(0, dbSize - 1);

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
void expDebug(ISse<>& sse, const Db<>& db, Range<Kw> query) {
    // setup
    sse.setup(KEY_LEN, db);

    // search
    std::vector<Doc<>> results = sse.search(query);
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

    sse.clear();
}


void exp1(ISse<>& sse, long dbSize) {
    if (dbSize == 0) {
        return;
    }
    Db<> db = createDb(dbSize, true, true);

    // setup
    auto setupStart = std::chrono::high_resolution_clock::now();
    sse.setup(KEY_LEN, db);
    auto setupEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> setupElapsed = setupEnd - setupStart;
    std::cout << "Setup time: " << setupElapsed.count() * 1000 << " ms" << std::endl;
    std::cout << std::endl;

    // search
    for (long i = 0; i <= std::log2(dbSize); i++) {
        Range<Kw> query {0, (long)std::pow(2, i) - 1};
        auto searchStartTime = std::chrono::high_resolution_clock::now();
        sse.search(query);
        auto searchEndTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> searchElapsed = searchEndTime - searchStartTime;
        std::cout << "Search time (size 2^" << std::log2(query.size()) << "): " << searchElapsed.count() * 1000
                  << " ms" << std::endl;
    }
    std::cout << std::endl;

    sse.clear();
}


void exp2(ISse<>& sse, long maxDbSize) {
    if (maxDbSize == 0) {
        return;
    }
    Range<Kw> query {0, 3};

    for (long i = 2; i <= std::log2(maxDbSize); i++) {
        long dbSize = std::pow(2, i);
        Db<> db = createDb(dbSize, true, true);

        // setup
        auto setupStart = std::chrono::high_resolution_clock::now();
        sse.setup(KEY_LEN, db);
        auto setupEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> setupElapsed = setupEnd - setupStart;
        std::cout << "Setup time (size 2^" << std::log2(dbSize) << "): " << setupElapsed.count() * 1000 << " ms"
                  << std::endl;

        // search
        auto searchStartTime = std::chrono::high_resolution_clock::now();
        sse.search(query);
        auto searchEndTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> searchElapsed = searchEndTime - searchStartTime;
        std::cout << "Search time (size 2^" << std::log2(dbSize) << "): " << searchElapsed.count() * 1000 << " ms"
                  << std::endl;
    }
    std::cout << std::endl;

    sse.clear();
}


void exp3(ISse<>& sse, long maxDbSize) {
    if (maxDbSize == 0) {
        return;
    }

    for (long i = 2; i <= std::log2(maxDbSize); i++) {
        long dbSize = std::pow(2, i);
        // two unique keywords, with all but one being 0 and the other being the max
        // thus all but one doc will be returned as false positives on a [1, n - 1] query (if the root node is the SRC)
        Db<> db;
        Kw kw1 = 0;
        Kw kw2 = dbSize - 1;
        Range<Kw> kwRange1 {kw1, kw1};
        Range<Kw> kwRange2 {kw2, kw2};
        for (long i = 0; i < dbSize - 1; i++) {
            db.push_back(DbEntry {Doc<>(i, kw1, Op::INS, kwRange1), kwRange1});
        }
        db.push_back(DbEntry {Doc<>(dbSize - 1, kw2, Op::INS, kwRange2), kwRange2});

        // setup
        auto setupStart = std::chrono::high_resolution_clock::now();
        sse.setup(KEY_LEN, db);
        auto setupEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> setupElapsed = setupEnd - setupStart;
        std::cout << "Setup time (size 2^" << std::log2(dbSize) << "): " << setupElapsed.count() * 1000 << " ms"
                  << std::endl;

        // search
        Range<Kw> query {1, dbSize - 1};
        auto searchStartTime = std::chrono::high_resolution_clock::now();
        sse.search(query);
        auto searchEndTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> searchElapsed = searchEndTime - searchStartTime;
        std::cout << "Search time (size 2^" << std::log2(dbSize) << "): " << searchElapsed.count() * 1000 << " ms"
                  << std::endl;
    }
    std::cout << std::endl;

    sse.clear();
}


int main() {
    long maxDbSizeExp;
    std::cout << "Enter database size (power of 2): ";
    std::cin >> maxDbSizeExp;
    long maxDbSize = std::pow(2, maxDbSizeExp);
    std::cout << std::endl;

    Pibas<> pibas;
    Nlogn<> nlogn;
    LogSrc<Pibas> logSrcPibas;
    LogSrc<Nlogn> logSrcNlogn;
    LogSrcI<Pibas> logSrcIPibas;
    LogSrcI<Nlogn> logSrcINlogn;
    LogSrcIStar logSrcIStar;
    Sda<Pibas<>> sdaPibas;
    Sda<Nlogn<>> sdaNlogn;
    Sda<LogSrc<Pibas>> sdaLogSrcPibas;
    Sda<LogSrc<Pibas>> sdaLogSrcNlogn;
    Sda<LogSrcI<Pibas>> sdaLogSrcIPibas;
    Sda<LogSrcI<Pibas>> sdaLogSrcINlogn;
    Sda<LogSrcIStar> sdaLogSrcIStar;

    /*
    //--------------------------------------------------------------------------
    // debugging experiment

    Range<Kw> query {3, 5};
    Db<> db = createDb(maxDbSize, true, true); // adjust params at will

    std::cout << "----------------------------- Debugging Experiment -----------------------------" << std::endl;
    std::cout << "DB size: 2^" << maxDbSizeExp                                                      << std::endl;
    std::cout << "Query  : "   << query                                                             << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "---------------- PiBas -----------------" << std::endl;
    std::cout << std::endl;
    expDebug(pibas, db, query);

    std::cout << "---------------- NlogN -----------------" << std::endl;
    std::cout << std::endl;
    expDebug(nlogn, db, query);

    std::cout << "------------ Log-SRC[PiBas] ------------" << std::endl;
    std::cout << std::endl;
    expDebug(logSrcPibas, db, query);

    std::cout << "------------ Log-SRC[NlogN] ------------" << std::endl;
    std::cout << std::endl;
    expDebug(logSrcNlogn, db, query);

    std::cout << "----------- Log-SRC-i[PiBas] -----------" << std::endl;
    std::cout << std::endl;
    expDebug(logSrcIPibas, db, query);

    std::cout << "----------- Log-SRC-i[NlogN] -----------" << std::endl;
    std::cout << std::endl;
    expDebug(logSrcINlogn, db, query);

    std::cout << "-------------- Log-SRC-i* --------------" << std::endl;
    std::cout << std::endl;
    expDebug(logSrcIStar, db, query);

    std::cout << "-------------- SDa[PiBas] --------------" << std::endl;
    std::cout << std::endl;
    expDebug(sdaPibas, db, query);

    std::cout << "-------------- SDa[NlogN] --------------" << std::endl;
    std::cout << std::endl;
    expDebug(sdaNlogn, db, query);

    std::cout << "--------- SDa[Log-SRC[PiBas]] ----------" << std::endl;
    std::cout << std::endl;
    expDebug(sdaLogSrcPibas, db, query);

    std::cout << "--------- SDa[Log-SRC[NlogN]] ----------" << std::endl;
    std::cout << std::endl;
    expDebug(sdaLogSrcNlogn, db, query);

    std::cout << "-------- SDa[Log-SRC-i[PiBas]] ---------" << std::endl;
    std::cout << std::endl;
    expDebug(sdaLogSrcIPibas, db, query);

    std::cout << "-------- SDa[Log-SRC-i[NlogN]] ---------" << std::endl;
    std::cout << std::endl;
    expDebug(sdaLogSrcINlogn, db, query);

    std::cout << "----------- SDa[Log-SRC-i*] ------------" << std::endl;
    std::cout << std::endl;
    expDebug(sdaLogSrcIStar, db, query);

    db.clear();
    */

    //--------------------------------------------------------------------------
    // experiment 1

    std::cout << "--------------------------------- Experiment 1 ---------------------------------" << std::endl;
    std::cout << "DB size: 2^" << maxDbSizeExp                                                      << std::endl;
    std::cout << "Query  : varied"                                                                  << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "---------------- PiBas -----------------" << std::endl;
    std::cout << std::endl;
    exp1(pibas, maxDbSize);

    std::cout << "---------------- NlogN -----------------" << std::endl;
    std::cout << std::endl;
    exp1(nlogn, maxDbSize);

    std::cout << "------------ Log-SRC[PiBas] ------------" << std::endl;
    std::cout << std::endl;
    exp1(logSrcPibas, maxDbSize);

    std::cout << "------------ Log-SRC[NlogN] ------------" << std::endl;
    std::cout << std::endl;
    exp1(logSrcNlogn, maxDbSize);

    std::cout << "----------- Log-SRC-i[PiBas] -----------" << std::endl;
    std::cout << std::endl;
    exp1(logSrcIPibas, maxDbSize);

    std::cout << "----------- Log-SRC-i[NlogN] -----------" << std::endl;
    std::cout << std::endl;
    exp1(logSrcINlogn, maxDbSize);

    std::cout << "-------------- Log-SRC-i* --------------" << std::endl;
    std::cout << std::endl;
    exp1(logSrcIStar, maxDbSize);

    std::cout << "-------------- SDa[PiBas] --------------" << std::endl;
    std::cout << std::endl;
    exp1(sdaPibas, maxDbSize);

    std::cout << "-------------- SDa[NlogN] --------------" << std::endl;
    std::cout << std::endl;
    exp1(sdaNlogn, maxDbSize);

    std::cout << "--------- SDa[Log-SRC[PiBas]] ----------" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrcPibas, maxDbSize);

    std::cout << "--------- SDa[Log-SRC[NlogN]] ----------" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrcNlogn, maxDbSize);

    std::cout << "-------- SDa[Log-SRC-i[PiBas]] ---------" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrcIPibas, maxDbSize);

    std::cout << "-------- SDa[Log-SRC-i[NlogN]] ---------" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrcINlogn, maxDbSize);

    std::cout << "----------- SDa[Log-SRC-i*] ------------" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrcIStar, maxDbSize);

    //--------------------------------------------------------------------------
    // experiment 2

    std::cout << "--------------------------------- Experiment 2 ---------------------------------" << std::endl;
    std::cout << "DB size: varied, up to 2^" << maxDbSizeExp                                        << std::endl;
    std::cout << "Query  : 0-3"                                                                     << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "---------------- PiBas -----------------" << std::endl;
    std::cout << std::endl;
    exp2(pibas, maxDbSize);

    std::cout << "---------------- NlogN -----------------" << std::endl;
    std::cout << std::endl;
    exp2(nlogn, maxDbSize);

    std::cout << "------------ Log-SRC[PiBas] ------------" << std::endl;
    std::cout << std::endl;
    exp2(logSrcPibas, maxDbSize);

    std::cout << "------------ Log-SRC[NlogN] ------------" << std::endl;
    std::cout << std::endl;
    exp2(logSrcNlogn, maxDbSize);

    std::cout << "----------- Log-SRC-i[PiBas] -----------" << std::endl;
    std::cout << std::endl;
    exp2(logSrcIPibas, maxDbSize);

    std::cout << "----------- Log-SRC-i[NlogN] -----------" << std::endl;
    std::cout << std::endl;
    exp2(logSrcINlogn, maxDbSize);

    std::cout << "-------------- Log-SRC-i* --------------" << std::endl;
    std::cout << std::endl;
    exp2(logSrcIStar, maxDbSize);

    std::cout << "-------------- SDa[PiBas] --------------" << std::endl;
    std::cout << std::endl;
    exp2(sdaPibas, maxDbSize);

    std::cout << "-------------- SDa[NlogN] --------------" << std::endl;
    std::cout << std::endl;
    exp2(sdaNlogn, maxDbSize);

    std::cout << "--------- SDa[Log-SRC[PiBas]] ----------" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrcPibas, maxDbSize);

    std::cout << "--------- SDa[Log-SRC[NlogN]] ----------" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrcNlogn, maxDbSize);

    std::cout << "-------- SDa[Log-SRC-i[PiBas]] ---------" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrcIPibas, maxDbSize);

    std::cout << "-------- SDa[Log-SRC-i[NlogN]] ---------" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrcINlogn, maxDbSize);

    std::cout << "----------- SDa[Log-SRC-i*] ------------" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrcIStar, maxDbSize);
    
    //--------------------------------------------------------------------------
    // experiment 3

    std::cout << "--------------------------------- Experiment 3 ---------------------------------" << std::endl;
    std::cout << "DB size: 2^" << maxDbSizeExp                                                      << std::endl;
    std::cout << "Query  : high false positives"                                                    << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "------------ Log-SRC[PiBas] ------------" << std::endl;
    std::cout << std::endl;
    exp3(logSrcPibas, maxDbSize);

    std::cout << "------------ Log-SRC[NlogN] ------------" << std::endl;
    std::cout << std::endl;
    exp3(logSrcNlogn, maxDbSize);

    std::cout << "----------- Log-SRC-i[PiBas] -----------" << std::endl;
    std::cout << std::endl;
    exp3(logSrcIPibas, maxDbSize);

    std::cout << "----------- Log-SRC-i[NlogN] -----------" << std::endl;
    std::cout << std::endl;
    exp3(logSrcINlogn, maxDbSize);
}
