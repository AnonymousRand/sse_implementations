#include <chrono>
#include <cmath>

#include "log_src.h"
#include "log_src_i.h"
#include "log_src_i_loc.h"
#include "pi_bas.h"
#include "sda.h"


Db<> createDb(long dbSize, bool isRandom, bool hasDeletions) {
    if (dbSize == 0) {
        return Db<> {};
    }
    Db<> db;
    std::uniform_int_distribution<long> dist(0, dbSize - 1);

    db.push_back(DbEntry {Doc {0, 4, Op::INS}, Range<Kw> {4, 4}});
    Id minId = 1;
    Id maxId = dbSize - 1;
    if (hasDeletions) {
        // delete the document with keyword 4
        db.push_back(DbEntry {Doc {0, 4, Op::DEL}, Range<Kw> {4, 4}});
        maxId = dbSize - 2;
    }

    // add in debugging experiment docs if we have the space to
    if (maxId - minId >= 1) {
        db.push_back(DbEntry {Doc {1, 3, Op::INS}, Range<Kw> {3, 3}});
        minId++;
    }
    if (maxId - minId >= 1) {
        db.push_back(DbEntry {Doc {2, 5, Op::INS}, Range<Kw> {5, 5}});
        minId++;
    }

    if (isRandom) {
        // fill the rest with random keywords
        for (Id id = minId; id <= maxId; id++) {
            Kw kw = dist(RNG);
            db.push_back(DbEntry {Doc {id, kw, Op::INS}, Range<Kw> {kw, kw}});
        }
    } else {
        for (Id id = minId; id <= maxId; id++) {
            // make keywords and ids inversely proportional to test sorting of Log-SRC-i's index 2
            // and make them non-contiguous to test Log-SRC as well
            Kw kw = (dbSize - id) * 2;
            db.push_back(DbEntry {Doc {id, kw, Op::INS}, Range<Kw> {kw, kw}});
        }
    }

    return db;
}


// experiment for debugging with fixed query and printed results
void expDebug(ISse<>& sse, long dbSize, Range<Kw> query) {
    if (dbSize == 0) {
        return;
    }
    Db<> db = createDb(dbSize, true, true); // adjust params at will

    // setup
    sse.setup(KEY_LEN, db);

    // search
    std::vector<Doc> results = sse.search(query);
    std::cout << "Results (id,kw,op):" << std::endl;
    for (Doc result : results) {
        Kw kw = result.getKw();
        if (query.contains(kw)) {
            std::cout << result << " with keyword " << kw << std::endl;
        } else {
            std::cout << result << " with keyword " << kw << " (false positive)" << std::endl;
        }
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
        long kw1 = 0;
        long kw2 = dbSize - 1;
        for (long i = 0; i < dbSize - 1; i++) {
            db.push_back(DbEntry {Doc(i, kw1, Op::INS), Range<Kw> {kw1, kw1}});
        }
        db.push_back(DbEntry {Doc(dbSize - 1, kw2, Op::INS), Range<Kw> {kw2, kw2}});

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

    PiBas<> piBas;
    LogSrc<PiBas> logSrc;
    LogSrcI<PiBas> logSrcI;
    LogSrcILoc<PiBasLoc> logSrcILoc;
    Sda<PiBas<>> sdaPiBas;
    Sda<LogSrc<PiBas>> sdaLogSrc;
    Sda<LogSrcI<PiBas>> sdaLogSrcI;
    Sda<LogSrcILoc<PiBasLoc>> sdaLogSrcILoc;

    /////////////////////////// debugging experiment ///////////////////////////

    Range<Kw> query {3, 5};

    std::cout << "----------------------------- Debugging Experiment -----------------------------" << std::endl;
    std::cout << "DB size: 2^" << maxDbSizeExp                                                      << std::endl;
    std::cout << "Query  : "   << query                                                             << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "---------------- PiBas -----------------" << std::endl;
    std::cout << std::endl;
    expDebug(piBas, maxDbSize, query);

    std::cout << "------------ Log-SRC[PiBas] ------------" << std::endl;
    std::cout << std::endl;
    expDebug(logSrc, maxDbSize, query);

    std::cout << "----------- Log-SRC-i[PiBas] -----------" << std::endl;
    std::cout << std::endl;
    expDebug(logSrcI, maxDbSize, query);

    std::cout << "---------- Log-SRC-i*[PiBas] -----------" << std::endl;
    std::cout << std::endl;
    expDebug(logSrcILoc, maxDbSize, query);

    std::cout << "-------------- SDa[PiBas] --------------" << std::endl;
    std::cout << std::endl;
    expDebug(sdaPiBas, maxDbSize, query);

    std::cout << "--------- SDa[Log-SRC[PiBas]] ----------" << std::endl;
    std::cout << std::endl;
    expDebug(sdaLogSrc, maxDbSize, query);

    std::cout << "-------- SDa[Log-SRC-i[PiBas]] ---------" << std::endl;
    std::cout << std::endl;
    expDebug(sdaLogSrcI, maxDbSize, query);

    std::cout << "-------- SDa[Log-SRC-i*[PiBas]] --------" << std::endl;
    std::cout << std::endl;
    expDebug(sdaLogSrcILoc, maxDbSize, query);

    /////////////////////////////// experiment 1 ///////////////////////////////

    std::cout << "--------------------------------- Experiment 1 ---------------------------------" << std::endl;
    std::cout << "DB size: 2^" << maxDbSizeExp                                                      << std::endl;
    std::cout << "Query  : varied"                                                                  << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "---------------- PiBas -----------------" << std::endl;
    std::cout << std::endl;
    exp1(piBas, maxDbSize);

    std::cout << "------------ Log-SRC[PiBas] ------------" << std::endl;
    std::cout << std::endl;
    exp1(logSrc, maxDbSize);

    std::cout << "----------- Log-SRC-i[PiBas] -----------" << std::endl;
    std::cout << std::endl;
    exp1(logSrcI, maxDbSize);

    std::cout << "---------- Log-SRC-i*[PiBas] -----------" << std::endl;
    std::cout << std::endl;
    exp1(logSrcILoc, maxDbSize);

    std::cout << "-------------- SDa[PiBas] --------------" << std::endl;
    std::cout << std::endl;
    exp1(sdaPiBas, maxDbSize);

    std::cout << "--------- SDa[Log-SRC[PiBas]] ----------" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrc, maxDbSize);

    std::cout << "-------- SDa[Log-SRC-i[PiBas]] ---------" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrcI, maxDbSize);

    std::cout << "-------- SDa[Log-SRC-i*[PiBas]] --------" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrcILoc, maxDbSize);

    /////////////////////////////// experiment 2 ///////////////////////////////

    std::cout << "--------------------------------- Experiment 2 ---------------------------------" << std::endl;
    std::cout << "DB size: varied, up to 2^" << maxDbSizeExp                                        << std::endl;
    std::cout << "Query  : 0-3"                                                                     << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "---------------- PiBas -----------------" << std::endl;
    std::cout << std::endl;
    exp2(piBas, maxDbSize);

    std::cout << "------------ Log-SRC[PiBas] ------------" << std::endl;
    std::cout << std::endl;
    exp2(logSrc, maxDbSize);

    std::cout << "----------- Log-SRC-i[PiBas] -----------" << std::endl;
    std::cout << std::endl;
    exp2(logSrcI, maxDbSize);

    std::cout << "---------- Log-SRC-i*[PiBas] -----------" << std::endl;
    std::cout << std::endl;
    exp2(logSrcILoc, maxDbSize);

    std::cout << "-------------- SDa[PiBas] --------------" << std::endl;
    std::cout << std::endl;
    exp2(sdaPiBas, maxDbSize);

    std::cout << "--------- SDa[Log-SRC[PiBas]] ----------" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrc, maxDbSize);

    std::cout << "-------- SDa[Log-SRC-i[PiBas]] ---------" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrcI, maxDbSize);

    std::cout << "-------- SDa[Log-SRC-i*[PiBas]] --------" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrcILoc, maxDbSize);
    
    /////////////////////////////// experiment 3 ///////////////////////////////

    std::cout << "--------------------------------- Experiment 3 ---------------------------------" << std::endl;
    std::cout << "DB size: 2^" << maxDbSizeExp                                                      << std::endl;
    std::cout << "Query  : high false positives"                                                    << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "------------ Log-SRC[PiBas] ------------" << std::endl;
    std::cout << std::endl;
    exp3(logSrc, maxDbSize);

    std::cout << "----------- Log-SRC-i[PiBas] -----------" << std::endl;
    std::cout << std::endl;
    exp3(logSrcI, maxDbSize);
}
